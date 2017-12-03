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

#include <QHostAddress>
#include <stdio.h>
#include <string.h>
#include "EQuilibre/Network/Connection.h"
#include "EQuilibre/Core/Message.h"
#include "EQuilibre/Network/Packet.h"
#include "EQuilibre/Network/PacketQueue.h"
#include "EQuilibre/Network/Session.h"

FragmentState::FragmentState()
{
    m_currentSize = m_totalSize = 0;
}

FragmentState::~FragmentState()
{
}

bool FragmentState::complete() const
{
    return m_currentPacket.data() && (m_currentSize == m_totalSize);
}

void FragmentState::addFragment(Packet &packet)
{
    // First fragment that contains the total size.
    bool firstFragment = !m_currentPacket.data();
    PacketStream inStream(packet);
    if(firstFragment)
    {
        m_currentSize = 0;
        m_totalSize = inStream.readBE32();
        m_currentPacket.setType(SM_ApplicationPacket);
        m_currentPacket.setSize(m_totalSize);
    }

    PacketStream outStream(m_currentPacket);
    outStream.setOffset(m_currentSize);

    uint32_t fragmentSize = inStream.left();
    Q_ASSERT(outStream.left() >= fragmentSize);
    memcpy(outStream.current(), inStream.current(), fragmentSize);
    m_currentSize += fragmentSize;
}

bool FragmentState::assemble(Packet &packet)
{
    if(!complete())
        return false;
    m_currentPacket.moveTo(packet);
    m_currentSize = 0;
    m_totalSize = 0;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

Connection::Connection(Clock *clock, Log *log, QObject *parent) : QObject(parent)
{
    m_clock = clock;
    m_log = log;
    m_connectTimer = clock->create(3 * 1000);
    m_sessionID = 0x26ec5075; // XXX Should it be random?
    m_session = NULL;
    m_state = eConnDisconnected;
    QObject::connect(m_connectTimer, SIGNAL(elapsed()), this, SLOT(disconnect()));
}

Connection::~Connection()
{
    disconnect();
}

ConnectionState Connection::state() const
{
    return m_state;
}

void Connection::setState(ConnectionState newState)
{
    if(newState != m_state)
    {
        m_state = newState;
        emit stateChanged(newState);
    }
}

bool Connection::connect(const QHostAddress &host, int port)
{
    if(m_state != eConnDisconnected)
        return (m_state == eConnConnected);
    QScopedPointer<Session> session(new Session(m_clock, m_log, host, port));
    
    // Send session initiation request.
    PacketSlot *slot = session->enqueueNonSeq(SM_SessionRequest, 12);
    if(!slot)
        return false;
    PacketStream stream(slot->packet());
    uint32_t unknownA = 2;
    uint32_t maxLength = 0x200;
    stream.writeBE32(unknownA);
    stream.writeBE32(m_sessionID);
    stream.writeBE32(maxLength);
    slot->setReady();
    
    // Do not wait for a response.
    m_session = session.take();
    setState(eConnConnecting);
    m_connectTimer->start();
    QObject::connect(m_session, SIGNAL(packetReceived(Packet&)),
                     this, SLOT(handlePacket(Packet&)));
    return true;
}

void Connection::disconnect()
{
    if((m_state == eConnDisconnected) || (m_state == eConnDisconnecting))
        return;
    setState(eConnDisconnecting);
    // Send a session disconnecting packet before freeing the connection.
    if(m_state == eConnConnected)
        sendSessionDisconnect(m_sessionID);
    m_session->deleteLater();
    m_session = NULL;
    setState(eConnDisconnected);
    m_connectTimer->clear();
    emit disconnected();
}

void Connection::handlePacket(Packet &packet)
{
    if(m_state == eConnConnecting)
    {
        uint32_t crcKey = 0;
        uint32_t maxLengthResp = 0;
        uint8_t format = 0;    
        if(extractSessionResponse(packet, crcKey, maxLengthResp, format))
        {
            // Update the session with the parameters from the response.
            m_session->setCRCKey(crcKey);
            m_session->setCompressed(format & SessionCompressed);
            setState(eConnConnected);
            m_connectTimer->clear();
            emit connected();
        }
        return;
    }
    else if(m_state != eConnConnected)
    {
        return;
    }
    
    MessageArray messages;
    NetworkResult result = processPacket(packet, messages);
    if(result == NetSuccess)
    {
        for(uint32_t i = 0; i < messages.size(); i++)
        {
            Message *msg = messages[i];
            if(!handleMessage(*msg))
                emit messageReceived(*msg);
            delete msg;
        }
    }
}

bool Connection::handleMessage(Message &msg)
{
    return false;
}

NetworkResult Connection::send(Message &message)
{
    if(!m_session)
        return NetDisconnected;

    // Determine the message opcode and encoded size.
    MessageInfo info;
    if(!info.findByID(message.id()))
    {
        qDebug("Could not get information for message type %d.", message.id());
        return NetInvalidPacket;
    }

    // Encode the message including the leading opcode.
    PacketSlot *slot = m_session->enqueueSeq(SM_ApplicationPacket, message.packetSize());
    if(!slot)
    {
        qDebug("warning: could not enqueue outgoing message 0x%x", info.code);
        return NetDisconnected;
    }
    PacketStream stream(slot->packet());
    stream.writeLE16(info.code);
    if(!message.encode(stream))
    {
        qDebug("Could not encode message of type %s (%d).",
               info.name, info.id);
        return NetInvalidPacket;
    }
    slot->setReady();
    return NetSuccess;
}

NetworkResult Connection::sendEmpty(MessageID id)
{
    if(!m_session)
        return NetDisconnected;
    MessageInfo info;
    if(!info.findByID(id))
        return NetInvalidPacket;
    PacketSlot *slot = m_session->enqueueSeq(SM_ApplicationPacket, 2);
    if(!slot)
    {
        qDebug("warning: could not enqueue outgoing message 0x%x", info.code);
        return NetDisconnected;
    }
    PacketStream stream(slot->packet());
    stream.writeLE16(info.code);
    slot->setReady();
    return NetSuccess;
}

static bool analyzePacket(BufferStream &stream, MessageInfo &info)
{
    // Get the message opcode.
    if(stream.left() < 2)
        return false;
    uint16_t opcode = stream.readLE16();

    // Decode the message type.
    return info.findByCode(opcode);
}

static bool messageFromStream(BufferStream &stream, MessageInfo &info, Message* &msgOut)
{
    QScopedPointer<Message> msgData;
    msgOut = NULL;
    msgData.reset(Message::create(info.id));
    if((msgData->flags() & Message::HasBody) && !msgData->decode(stream))
    {
        qDebug("Could not decode message of type: %s (0x%x)", info.name, info.code);
        return false;
    }
    msgOut = msgData.take();
    return true;
}

static bool messagesFromStream(BufferStream &stream, MessageArray &messages)
{
    MessageInfo info;
    if(!analyzePacket(stream, info))
    {
        return false;
    }
    else if(info.id != AM_AppCombined)
    {
        Message *msg = NULL;
        if(!messageFromStream(stream, info, msg))
            return false;
        messages.append(msg);
        return true;
    }
    
    bool success = true;
    while(stream.left())
    {
        // Read sub-message size.
        uint8_t subMsgSize = stream.read8();
        Q_ASSERT(subMsgSize <= stream.left());
        
        // Read sub-message.
        BufferStream subStream(stream.current(), subMsgSize);
        if(!messagesFromStream(subStream, messages))
        {
            success = false;
            qDebug("warning: could not read sub-message at offset %d", stream.offset());
        }
        stream.skip(subMsgSize);
    }
    return success;
}

NetworkResult Connection::processPacket(Packet &packetIn, MessageArray &messages)
{
    // The server can send application packets disguised as session packets.
    PacketType type = packetIn.type();
    PacketStream stream(packetIn);
    if(type & 0xff00)
    {
        // Swap the opcode which was read as big endian even though it is not.
        uint16_t opcode = BufferStream::swap16((uint16_t)type);
        MessageInfo info;
        if(!info.findByCode(opcode))
            return dropPacket(packetIn);
        Message *msg = NULL;
        if(!messageFromStream(stream, info, msg))
            return dropPacket(packetIn);
        messages.append(msg);
        return NetSuccess;
    }
    
    // Decide what to do with the packet.
    switch(type)
    {
    default:
        qDebug("Unexpected session packet: 0x%x", type);
        return dropPacket(packetIn);
    case SM_ApplicationPacket:
        if(!messagesFromStream(stream, messages))
            return dropPacket(packetIn);
        return NetSuccess;
    case SM_Fragment:
        m_fragmentState.addFragment(packetIn);
        if(m_fragmentState.complete())
        {
            Packet assembledPacket;
            if(!m_fragmentState.assemble(assembledPacket))
                return dropPacket(assembledPacket);
            stream = PacketStream(assembledPacket);
            if(!messagesFromStream(stream, messages))
                return dropPacket(assembledPacket);
            return NetSuccess;
        }
        break;
    case SM_SessionDisconnect:
        disconnect();
        return NetDisconnected;
    }
    
    // Ignoring this message, do not return it to the caller.
    return NetAgain;
}

NetworkResult Connection::dropPacket(Packet &packetIn)
{
    qDebug("warning: dropping packet (type=0x%d, seq=%d, size=%d)",
           packetIn.type(), packetIn.size(), packetIn.seqNum());
    return NetInvalidPacket;
}

void Connection::sendSessionDisconnect(uint32_t sessionID)
{
    PacketSlot *slot = m_session->enqueueNonSeq(SM_SessionDisconnect, 6);
    if(slot)
    {
        PacketStream stream(slot->packet());
        uint16_t unknownA = 6;
        stream.writeBE32(sessionID);
        stream.writeBE16(unknownA);
        slot->setReady();
    }
}

bool Connection::extractSessionResponse(Packet &response,
                                        uint32_t &crcKey,
                                        uint32_t &maxLength,
                                        uint8_t &format)
{
    if(response.type() != SM_SessionResponse)
        return false;
    PacketStream stream(response);
    if(stream.left() < 16)
        return false;
    
    // Match the session ID with the expected ID.
    uint32_t actualSessionID = stream.readBE32();
    if(m_sessionID != actualSessionID)
        return false;
    
    // Extract the other fields from the response.
    crcKey = stream.readBE32();
    uint8_t unknownA = stream.read8();
    format = stream.read8();
    uint8_t unknownB = stream.read8();
    maxLength = stream.readBE32();
    uint32_t unknownC = stream.readBE32();
    return true;
}

