// Copyright (C) 2013 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <QElapsedTimer>
#include <QTimer>
#include <QUdpSocket>
#include <string.h>
#include "EQuilibre/Network/Session.h"
#include "EQuilibre/Network/Datagram.h"
#include "EQuilibre/Network/Packet.h"
#include "EQuilibre/Network/PacketQueue.h"
#include "EQuilibre/Core/Log.h"

Session::Session(Clock *clock, Log *log, const QHostAddress &host, int port,
                 QObject *parent) : QObject(parent)
{
    m_clock = clock;
    m_log = log;
    m_crcKey = 0;
    m_compressed = false;
    m_nextAckIn = 0;
    m_nextAckOut = 0;
    m_nextSeqIn = 0;
    m_nextSeqOut = 0;
    m_receiveQueue = new PacketQueue(6, false);
    m_receiveSeqQueue = new PacketQueue(8, true);
    m_sendQueue = new PacketQueue(6, false);
    m_sendSeqQueue = new PacketQueue(8, true);
    m_retransmitCooldown = clock->create(100);
    m_socket = new QUdpSocket(this);
    m_socket->connectToHost(host, port);
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(receivePackets()));
    connect(m_clock, SIGNAL(ticked()), this, SLOT(networkTick()));
}

Session::~Session()
{
    delete m_sendSeqQueue;
    delete m_sendQueue;
    delete m_receiveSeqQueue;
    delete m_receiveQueue;
}

bool Session::compressed() const
{
    return m_compressed;    
}

void Session::setCompressed(bool enable)
{
    m_compressed = enable;
}

uint32_t Session::CRCKey() const
{
    return m_crcKey;
}

void Session::setCRCKey(uint32_t newKey)
{
    m_crcKey = newKey;
}

int Session::receiveDatagrams(int maxDatagrams)
{
    uint8_t buffer[128 * 1024];
    Datagram datagram(buffer, sizeof(buffer));
    int received = 0;
    
    // Receive as many packets as possible until the socket buffers are empty.
    NetworkResult result = receiveDatagram(datagram);
    while((result != NetAgain) && (result != NetDisconnected))
    {
        if(result == NetSuccess)
        {
            enqueueReceivedPacket(datagram);
            received++;
            if(received >= maxDatagrams)
                break;
        }
        result = receiveDatagram(datagram);
    }
    
    // Collect any outgoing packet we received acks for.
    m_sendSeqQueue->collect();
    
    //if(received)
    //    logMessage("Received %d datagrams", received);
    return received;
}

NetworkResult Session::receiveDatagram(Datagram &datagram)
{
    qint64 size = m_socket->read((char *)datagram.buffer(), datagram.bufferSize());
    if(size > 0)
    {
        datagram.clear();
        datagram.setSize((uint32_t)size);
        if(!datagram.read(m_crcKey, m_compressed))
        {
            return NetInvalidPacket;
        }
        return NetSuccess;
    }
    return NetAgain;
}

bool Session::enqueueReceivedPacket(Datagram &datagram)
{
    // Handle certain packet types in a special way.
    PacketType type = (PacketType)datagram.type();
    BufferStream stream(datagram.buffer() + datagram.offset(), datagram.size());
    if(type == SM_Ack)
    {
        // Release outgoing packets that have been acked but do not enqueue Ack packets.
        uint16_t seqNum = stream.readBE16();
        PacketQueue &q(*m_sendSeqQueue);
        for(PacketIterator I = q.begin(), E = q.end(); I != E; ++I)
        {
            if((I->seqNum() > seqNum) || !I->isProcessed())
            {
                break;
            }
            I->release();
            m_nextAckIn = (I->seqNum() + 1);
        }
        return true;
    }
    else if(type == SM_Combined)
    {
        // Do not enqueue combined packets, only their sub-packets.
        bool enqueueAll = true;
        while(stream.left())
        {
            // Read sub-message size.
            uint8_t subMsgSize = stream.read8();
            Q_ASSERT(subMsgSize <= stream.left());
            
            // Read sub-message.
            Datagram subDatagram(stream.current(), subMsgSize);
            subDatagram.setSize(subMsgSize);
            if(subDatagram.readUnwrapped())
            {
                enqueueAll &= enqueueReceivedPacket(subDatagram);
            }
            else
            {
                enqueueAll = false;
                if(m_log)
                    m_log->writeLine("warning: could not read sub-message at offset %d", stream.offset());
            }
            stream.skip(subMsgSize);
        }
        return enqueueAll;
    }
    
    PacketSlot *slot = NULL; 
    if(datagram.hasSeqNum())
    {
        uint16_t seqNum = datagram.seqNum();
        slot = m_receiveSeqQueue->allocateAt(seqNum).slot();
        if(!slot)
        {
            PacketIterator I = m_receiveSeqQueue->findAt(seqNum);
            if(I)
            {
                if(m_log)
                    m_log->writeLine("Ignoring duplicate of packet %d.", seqNum);
            }
            else
            {
                if(m_log)
                    m_log->writeLine("Receive queue is full, dropping packet %d.", seqNum);
            }
            return false;
        }
    }
    else
    {
        slot = m_receiveQueue->allocate().slot();
        if(!slot)
        {
            if(m_log)
                m_log->writeLine("Receive queue is full, dropping packet.");
            return false;
        }
    }
    slot->copyDatagram(datagram);
    return true;
}

void Session::receivePackets()
{
    // Limit the maximum number of datagrams to receive in one go.
    // Otherwise on a very fast connection it is possible to get too many
    // packets and fill the receive queue before being able to handle them.
    int received = receiveDatagrams(m_maxDatagramBatch);
    int toAck = 0;
    uint16_t maxSeqToAck = 0;
    while(received > 0)
    {
        processReceivedPackets();
        
        // For the same reason, do not wait the next network tick to
        // ack the packets we received if they start piling up in the queue.
        toAck = pendingReceiveAck(maxSeqToAck);
        if(toAck >= m_maxDatagramBatch)
        {
            sendAck(maxSeqToAck);
        }
        
        received = receiveDatagrams(m_maxDatagramBatch);
    }
}

bool Session::findPendingPacketIn(PacketIterator &I)
{
    I = m_receiveQueue->begin();
    for(PacketIterator E = m_receiveQueue->end(); I != E; ++I)
    {
        if(I->setReady())
        {
            return true;
        }
    }
    
    I = m_receiveSeqQueue->findAt(m_nextSeqIn);
    if(I && I->setReady())
    {
        Q_ASSERT(m_nextSeqIn == I->seqNum());
        m_nextSeqIn++;
        return true;
    }
    
    return false;
}

int Session::processReceivedPackets()
{
    // Process as many packets as possible until the queues are empty.
    int processed = 0;
    while(true)
    {
        // Get an unprocessed packet from the queue.
        PacketIterator I;
        if(!findPendingPacketIn(I))
        {
            //logMessage("Could not find packet %d from the sequenced queue.", m_nextSeqIn);
            break;
        }
        processed++;
        
        // Pass it to whomever it may concern and dispose of it.
        //qDebug("received packet of type %d, size %d and seqnum %d",
        //       packet->type(), packet->size(), packet->seqNum());
        emit packetReceived(I->packet());
        if(I.isSequenced())
        {
            // Mark the packet for future acknowledgment.
            I->setProcessed();
        }
        else
        {
            // No need to ack non-sequenced packets, we can release it now.
            I->release();
        }
    }
    m_receiveQueue->collect();
    
    //if(processed)
    //    logMessage("Processed %d packets", processed);
    return processed;
}

bool Session::detectReceiveStall(uint16_t &stalledSeq, int &following)
{
    PacketQueue &q(*m_receiveSeqQueue);
    PacketIterator LastFree;
    for(PacketIterator I = q.begin(), E = q.end(); I != E; ++I)
    {
        if(LastFree)
        {
            if(I->isAllocated())
            {
                following++;
            }
        }
        else if(!I->isAllocated())
        {
            LastFree = I;
        }
    }
    
    if(LastFree && (following > 0))
    {
        stalledSeq = LastFree.seqNum();
        return true;
    }
    return false;
}

int Session::pendingReceiveAck(uint16_t &maxSeqNum)
{
    // Find the highest processed but not acked packet.
    PacketQueue &q(*m_receiveSeqQueue);
    int minProcessedSeqNum = 0x10000;
    int maxProcessedSeqNum = -1;
    for(PacketIterator I = q.begin(), E = q.end(); I != E; ++I)
    {
        if(I->isProcessed())
        {
            uint16_t seqNum = I->seqNum();
            minProcessedSeqNum = qMin(minProcessedSeqNum, (int)seqNum);
            maxProcessedSeqNum = qMax(maxProcessedSeqNum, (int)seqNum);
        }
        else if(maxProcessedSeqNum >= 0)
        {
            break;
        }
    }
    
    // No packet to ack.
    if(maxProcessedSeqNum < 0)
    {
        maxSeqNum = 0;
        return 0;
    }
    
    Q_ASSERT(minProcessedSeqNum == m_nextAckOut);
    maxSeqNum = (uint16_t)maxProcessedSeqNum;
    return (maxProcessedSeqNum - minProcessedSeqNum);
}

static PacketSlot * enqueue(PacketQueue *q, PacketType type, uint32_t size)
{
    PacketIterator I = q->allocate();
    PacketSlot *slot = I.slot();
    if(slot)
    {
        slot->newPacket(type, size);
    }
    return slot;
}

PacketSlot * Session::enqueueNonSeq(PacketType type, uint32_t size)
{
    return enqueue(m_sendQueue, type, size);
}

PacketSlot * Session::enqueueSeq(PacketType type, uint32_t size)
{
    return enqueue(m_sendSeqQueue, type, size);
}

bool Session::findPendingPacketOut(PacketIterator &I)
{
    I = m_sendQueue->begin();
    for(PacketIterator E = m_sendQueue->end(); I != E; ++I)
    {
        if(I->isReady())
        {
            return true;
        }
    }
    
    I = m_sendSeqQueue->findAt(m_nextSeqOut);
    if(I && I->isReady())
    {
        Q_ASSERT(m_nextSeqOut == I->seqNum());
        return true;
    }
    
    return false;
}

void Session::sendPendingPackets()
{
    // Send as many packets as possible until the queues are empty.
    while(true)
    {
        // Get an unsent packet from the queue.
        PacketIterator I;
        if(!findPendingPacketOut(I))
        {
            break;
        }
        
        // Send the packet.
        if(send(I->packet()) == NetAgain)
        {
            break;
        }
        
        if(I.isSequenced())
        {
            // Keep the packet until it is acknowledged.
            I->setProcessed();
            m_nextSeqOut++;
        }
        else
        {
            // No need to ack non-sequenced packets, we can release it now.
            I->release();
        }
    }
    m_sendQueue->collect();
}

NetworkResult Session::send(Packet &packet)
{
    uint8_t buffer[4096];
    Datagram datagram(buffer, sizeof(buffer));
    datagram.setType(packet.type());
    datagram.setHasSeqNum(packet.testFlag(Packet::Sequenced));
    datagram.setSeqNum(packet.seqNum());

    // Write the packet to the buffer, compressing it if needed.
    PacketStream inStream(packet);
    datagram.write(inStream, m_crcKey, m_compressed);

    // Send the packet.
    qint64 size = m_socket->write((char *)datagram.buffer(), datagram.size());
    if(size < datagram.size())
        return NetAgain;
    return NetSuccess;
}

void Session::sendAck(uint16_t maxSeqNum)
{
    // Send ack packet.
    Packet packet(SM_Ack, 2);
    PacketStream stream(packet);
    stream.writeBE16(maxSeqNum);
    send(packet);
    
    // Remove all processed packets from the queue.
    PacketQueue &q(*m_receiveSeqQueue);
    for(PacketIterator I = q.begin(), E = q.end(); I != E; ++I)
    {
        if(I->seqNum() <= maxSeqNum)
        {
            I->release();
        }
    }
    q.collect();
    
    //logMessage("Acked packets %d-%d", m_nextAckOut, maxSeqNum);
    m_nextAckOut = maxSeqNum + 1;
}

void Session::networkTick()
{
    // Send acknowledgements for packets we have received in order.
    uint16_t maxSeqToAck = 0;
    int toAck = pendingReceiveAck(maxSeqToAck);
    if(toAck > 0)
    {
        sendAck(maxSeqToAck);
    }
    
    // Ask for a retransmission of dropped packets.
    uint16_t stalledSeq = 0;
    int following = 0;
    if(!m_retransmitCooldown->active() && detectReceiveStall(stalledSeq, following))
    {
        Packet packet(SM_OutOfOrderAck, 2);
        PacketStream stream(packet);
        stream.writeBE16(stalledSeq);
        if(m_log)
            m_log->writeLine("Detected receive stall at #%d (%d packets following), asking for retransmit.", stalledSeq, following);
        send(packet);
        m_retransmitCooldown->start();
    }
    
    // Send packets that have been enqueued but not sent.
    sendPendingPackets();
}

////////////////////////////////////////////////////////////////////////////////

Timer::Timer(int durationMs, const char *name, Clock *parent) : QObject(parent)
{
    m_clock = parent;
    m_name = name;
    m_durationMs = durationMs;
    m_timeLeftMs = -1;
}

bool Timer::active() const
{
    return m_timeLeftMs >= 0;
}

void Timer::clear()
{
    m_timeLeftMs = -1;
}

void Timer::start()
{
    m_timeLeftMs = m_durationMs;
}

bool Timer::update(int elapsedMs)
{
    if(!active())
        return false;
    m_timeLeftMs -= elapsedMs;
    if(m_timeLeftMs > 0)
        return false;
    Log *log = m_clock ? m_clock->log() : NULL;
    if(log && m_name)
    {
        log->writeLine("Timer '%s' elapsed.", m_name);
    }
    emit elapsed();
    clear();
    return true;
}

////////////////////////////////////////////////////////////////////////////////

Clock::Clock(uint32_t tickMs, Log *log, QObject *parent) : QObject(parent)
{
    m_tickMs = tickMs;   
    m_log = log;
    m_tickTimer = new QTimer(this);
    m_tickTimer->start(tickMs);
    m_elapsed = new QElapsedTimer();
    m_elapsed->start();
    connect(m_tickTimer, SIGNAL(timeout()), this, SLOT(tick()));
}

Clock::~Clock()
{
    delete m_elapsed;
}

Log * Clock::log() const
{
    return m_log;
}

Timer * Clock::create(int durationMs, const char *name)
{
    Timer *t = new Timer(durationMs, name, this);
    m_timers.append(t);
    return t;
}

void Clock::tick()
{
    uint32_t elapsedMs = m_tickMs;
    if(m_elapsed->isValid())
    {
        elapsedMs = (uint32_t)m_elapsed->elapsed();
        //qDebug("Net tick: %d ms elapsed since last tick.", elapsedMs);
    }
    m_elapsed->start();
    foreach(Timer *timer, m_timers)
        timer->update(elapsedMs);
    emit ticked();
}
