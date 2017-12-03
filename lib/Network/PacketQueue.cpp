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

#include <QDebug>
#include "EQuilibre/Network/PacketQueue.h"

Packet & PacketSlot::packet()
{
    return m_packet;
}

uint16_t PacketSlot::seqNum() const
{
    return m_packet.seqNum();
}

bool PacketSlot::swapStates(PacketState oldState, PacketState newState)
{
    if(m_packet.state() == oldState)
    {
        m_packet.setState(newState);
        return true;
    }
    return false;
}

bool PacketSlot::isAllocated() const
{
    return m_packet.state() == ePacketAllocated;
}

bool PacketSlot::isReady() const
{
    return m_packet.state() == ePacketReady;
}

bool PacketSlot::isProcessed() const
{
    return m_packet.state() == ePacketProcessed;
}

bool PacketSlot::setAllocated()
{
    return swapStates(ePacketFree, ePacketAllocated);
}

bool PacketSlot::setReady()
{
    return swapStates(ePacketAllocated, ePacketReady);
}

bool PacketSlot::setProcessed()
{
    return swapStates(ePacketReady, ePacketProcessed);
}

bool PacketSlot::setAcked()
{
    return swapStates(ePacketProcessed, ePacketAcked);
}

bool PacketSlot::setFree()
{
    return swapStates(ePacketPendingFree, ePacketFree);
}

void PacketSlot::newPacket(PacketType type, uint32_t size)
{
    m_packet.setType(type);
    if(size <= InlineSize)
    {
        m_packet.setBuffer(m_data, size);
        m_packet.enableFlags(Packet::InlineBuffer);
    }
    else
    {
        m_packet.setSize(size);
    }
}

void PacketSlot::copyDatagram(Datagram &datagram)
{
    m_packet.fromDatagram(datagram);
    if(m_packet.size() <= InlineSize)
    {
        memcpy(m_data, m_packet.data(), m_packet.size());
        m_packet.setBuffer(m_data, m_packet.size());
        m_packet.enableFlags(Packet::InlineBuffer);
    }
    else
    {
        m_packet.copy();
    }
}

bool PacketSlot::release()
{
    if(m_packet.state() < ePacketAllocated)
        return false;
    m_packet.clear();
    m_packet.setState(ePacketPendingFree);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

PacketIterator::PacketIterator()
{
    m_queue = NULL;
    m_seq = 0;
    m_mask = 0;
}

PacketIterator::PacketIterator(PacketQueue *queue, uint16_t seq, uint16_t mask)
{
    m_queue = queue;
    m_seq = seq;
    m_mask = mask;
}

bool PacketIterator::isSequenced() const
{
    return m_queue ? m_queue->isSequenced() : false;
}

bool PacketIterator::isValid() const
{
    return (m_queue != NULL);
}

uint16_t PacketIterator::seqNum() const
{
    return m_seq;
}

PacketSlot * PacketIterator::slot() const
{
    if(!isValid())
        return NULL;
    uint16_t i = (m_seq & m_mask);
    return m_queue->slotAt(i);
}

PacketIterator::reference PacketIterator::operator*() const
{
    Q_ASSERT(isValid());
    return *slot();
}

PacketIterator::pointer PacketIterator::operator->() const
{
    Q_ASSERT(isValid());
    return slot();
}

PacketIterator & PacketIterator::operator++()
{
    if(m_queue)
    {
        m_seq++;
        if(m_queue->tail() == m_seq)
        {
            *this = PacketIterator();
        }
    }
    return *this;
}

PacketIterator & PacketIterator::operator=(const PacketIterator &other)
{
    m_queue = other.m_queue;
    m_seq = other.m_seq;
    m_mask = other.m_mask;
    return *this;
}

bool PacketIterator::operator==(const PacketIterator &other) const
{
    return (m_queue == other.m_queue) && (m_seq == other.m_seq);
}

bool PacketIterator::operator!=(const PacketIterator &other) const
{
    return !(*this == other);
}

PacketIterator::operator bool() const
{
    return isValid();
}

////////////////////////////////////////////////////////////////////////////////

PacketQueue::PacketQueue(int capacityBits, bool sequenced)
{
    // Do not allow queues larger than 2^16 elements to match the sequence size.
    m_capacityBits = (uint8_t)qMin(capacityBits, 16);
    m_sequenced = sequenced;
    
    uint32_t capacity = ((uint32_t)1 << m_capacityBits);
    m_capacityMask = (uint16_t)(capacity - 1);
    m_head = 0;
    m_tail = 0;
    m_slots.resize(capacity);
}

bool PacketQueue::isSequenced() const
{
    return m_sequenced;
}

PacketSlot * PacketQueue::slotAt(uint16_t index)
{
    if(index > m_slots.size())
        return NULL;
    return &m_slots[index];
}

uint16_t PacketQueue::head() const
{
    return m_head;
}

uint16_t PacketQueue::tail() const
{
    return m_tail;
}

PacketQueue::iterator PacketQueue::begin()
{
    return (m_head != m_tail) ? makeIterator(m_head) : end();
}

PacketQueue::iterator PacketQueue::end()
{
    return PacketIterator();
}

PacketQueue::iterator PacketQueue::makeIterator(uint16_t seqNum)
{
    return PacketIterator(this, seqNum, m_capacityMask);
}

PacketQueue::iterator PacketQueue::allocate()
{
    return allocateAt(m_tail);
}

PacketQueue::iterator PacketQueue::allocateAt(uint16_t seqNum)
{
    // Check whether the sequence is either in the past or is too far away in
    // the future for the queue using the distance from the head.
    uint16_t newTail = seqNum + 1;
    uint16_t newCount = newTail - m_head;
    if(newCount < (1 << m_capacityBits))
    {
        iterator I = makeIterator(seqNum);
        if(I->setAllocated())
        {
            // Update the tail position if needed.
            uint16_t oldCount = m_tail - m_head;
            if(newCount > oldCount)
            {
                Q_ASSERT((newTail & m_capacityMask) != (m_head & m_capacityMask));
                m_tail = newTail;
            }
            
            // Set the packet's sequence number.
            if(m_sequenced)
            {
                I->packet().setSeqNum(seqNum);
                I->packet().enableFlags(Packet::Sequenced);
            }
            return I;
        }
    }
    return end();
}

int PacketQueue::collect()
{
    int collected = 0;
    for(PacketIterator I = begin(), E = end(); I != E; ++I)
    {
        if(!I->setFree())
        {
            break;
        }
        m_head++;
        collected++;
    }
    return collected;
}

PacketQueue::iterator PacketQueue::findAt(uint16_t seqNum)
{
    uint16_t count = m_tail - m_head;
    uint16_t dist = seqNum - m_head;
    return (dist < count) ? makeIterator(seqNum) : end();
}
