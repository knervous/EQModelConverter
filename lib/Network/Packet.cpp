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
#include "EQuilibre/Network/Packet.h"
#include "EQuilibre/Network/Datagram.h"
#include "EQuilibre/Network/Session.h"

Packet::Packet()
{
    m_type = SM_Invalid;
    m_seqNum = 0;
    m_buffer = NULL;
    m_size = 0;
    m_flags = NoFlag;
    m_state = ePacketFree;
}

Packet::Packet(uint8_t *data, uint32_t size, bool copy)
{
    m_type = SM_Invalid;
    m_seqNum = 0;
    m_buffer = NULL;
    m_size = 0;
    m_flags = NoFlag;
    m_state = ePacketFree;
    if(copy)
    {
        setSize(size);
        memcpy(m_buffer, data, size);
    }
    else
    {
        setBuffer(data, size);
    }
}

Packet::Packet(PacketType type, uint32_t size)
{
    m_type = type;
    m_buffer = NULL;
    m_size = 0;
    m_seqNum = 0;
    m_flags = NoFlag;
    m_state = ePacketFree;
    setSize(size);
    memset(m_buffer, 0, size);
}

Packet::~Packet()
{
    clear();
}

void Packet::clear()
{
    if(testFlag(OwnsBuffer))
        delete [] m_buffer;
    m_type = SM_Invalid;
    m_buffer = NULL;
    m_size = 0;
    m_seqNum = 0;
    m_flags = NoFlag;
    m_state = ePacketFree;
}

void Packet::moveTo(Packet &target)
{
    target = *this;
    clearFlags(OwnsBuffer);
    clear();
}

PacketType Packet::type() const
{
    return (PacketType)m_type;
}

void Packet::setType(PacketType newType)
{
    m_type = newType;
}

uint16_t Packet::seqNum() const
{
    return m_seqNum;
}

void Packet::setSeqNum(uint16_t newSeq)
{
    m_seqNum = newSeq;
}

uint8_t * Packet::data() const
{
    return m_buffer; 
}

uint32_t Packet::size() const
{
    return m_size;
}

void Packet::setSize(uint32_t newSize)
{
    if(!newSize)
        return;
    uint8_t *newBuffer = new uint8_t[newSize];
    memset(newBuffer, 0, newSize);
    setBuffer(newBuffer, newSize);
    enableFlags(OwnsBuffer);
}

void Packet::setBuffer(uint8_t *data, uint32_t size)
{
    if(testFlag(OwnsBuffer))
    {
        delete [] m_buffer;
        clearFlags(OwnsBuffer);
    }
    m_buffer = data;
    m_size = size;
}

void Packet::copy()
{
    if(!testFlag(OwnsBuffer))
    {
        uint8_t *newBuffer = new uint8_t[m_size];
        memcpy(newBuffer, m_buffer, m_size);
        m_buffer = newBuffer;
        enableFlags(OwnsBuffer);
    }
}

Packet::Flags Packet::flags() const
{
    return (Packet::Flags)m_flags;
}

bool Packet::testFlag(Flags flags) const
{
    return (m_flags & flags);
}

void Packet::enableFlags(Flags newFlags)
{
    m_flags |= newFlags;
}

void Packet::clearFlags(Flags newFlags)
{
    m_flags &= ~newFlags;
}

PacketState Packet::state() const
{
    return (PacketState)m_state;   
}

void Packet::setState(PacketState newState)
{
    m_state = newState;
}

void Packet::fromDatagram(Datagram &datagram)
{
    uint8_t *data = datagram.buffer();
    if(data && datagram.size())
    {
        setBuffer(data + datagram.offset(), datagram.size());
        m_type = (PacketType)datagram.type();
        m_seqNum = datagram.seqNum();
        if(datagram.hasSeqNum())
            enableFlags(Sequenced);
    }
}

////////////////////////////////////////////////////////////////////////////////

PacketStream::PacketStream(Packet &packet) : BufferStream(packet.data(), packet.size())
{

}
