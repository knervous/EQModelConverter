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
#include <string.h>
#include "EQuilibre/Core/BufferStream.h"

BufferStream::BufferStream(uint8_t *buffer, uint32_t size)
{
    m_data = buffer;
    m_size = size;
    m_offset = 0;
}

uint32_t BufferStream::offset() const
{
    return m_offset;
}

void BufferStream::setOffset(uint32_t newOffset)
{
    if(newOffset < m_size)
        m_offset = newOffset;
}

uint32_t BufferStream::left() const
{
    return m_size - m_offset;
}

uint8_t * BufferStream::current() const
{
    return m_data + m_offset;
}

void BufferStream::skip(uint32_t bytes)
{
    if(left() >= bytes)
        m_offset += bytes;
}

char * BufferStream::readCString(uint32_t fixedSize)
{
    uint8_t *data = current();
    uint32_t maxSize = left();
    uint32_t pos = 0;
    if(fixedSize)
        maxSize = fixedSize;
    while(pos < maxSize)
    {
        if(data[pos] == 0)
        {
            if(fixedSize)
                m_offset += fixedSize;
            else
                m_offset += (pos + 1);
            return (char *)data;
        }
        pos++;
    }
    return NULL;
}

uint8_t BufferStream::read8()
{
    if(left() == 0)
        return 0xff;
    uint8_t val = read8(m_data, m_offset);
    m_offset += sizeof(uint8_t);
    return val;
}

uint16_t BufferStream::readBE16()
{
    if(left() < sizeof(uint16_t))
        return 0xffff;
    uint16_t val = readBE16(m_data, m_offset);
    m_offset += sizeof(uint16_t);
    return val;
}

uint16_t BufferStream::readLE16()
{
    if(left() < sizeof(uint16_t))
        return 0xffff;
    uint16_t val = readLE16(m_data, m_offset);
    m_offset += sizeof(uint16_t);
    return val;
}

uint32_t BufferStream::readBE32()
{
    if(left() < sizeof(uint32_t))
        return 0xffffffff;
    uint32_t val = readBE32(m_data, m_offset);
    m_offset += sizeof(uint32_t);
    return val;
}

uint32_t BufferStream::readLE32()
{
    if(left() < sizeof(uint32_t))
        return 0xffffffff;
    uint32_t val = readLE32(m_data, m_offset);
    m_offset += sizeof(uint32_t);
    return val;
}

float BufferStream::readFP32()
{
    if(left() < sizeof(uint32_t))
        return 0.0f; // XXX NaN?
    uint32_t val = readLE32(m_data, m_offset);
    m_offset += sizeof(uint32_t);
    return *(float *)&val;
}

unsigned BufferStream::peekZExtBits(int offset, int count)
{
    Q_ASSERT((offset + count) <= 32);
    uint64_t val = readLE32(m_data, m_offset);
    uint64_t mask = ((1 << count) - 1) << (uint64_t)offset;
    return (uint32_t)((val & mask) >> offset);
}

int BufferStream::peekSExtBits(int offset, int count)
{
    uint32_t val = peekZExtBits(offset, count);
    uint32_t sign = val & (1 << (count - 1));
    if((count == 32) || (sign == 0))
        return (int)val;
    uint32_t ones = ~((1 << count) - 1);
    uint32_t extVal = (val | ones);
    return (int)extVal;
}

bool BufferStream::writeCString(const char *str, uint32_t fixedSize)
{
    size_t srcSize = str ? strlen(str) + 1 : 1;
    size_t dstSize = fixedSize ? fixedSize : srcSize;
    if(left() < dstSize)
        return false;
    if(str)
    {
        memcpy(current(), str, srcSize - 1);
    }
    write8(0, current(), dstSize - 1);
    m_offset += dstSize;
    return true;
}

bool BufferStream::write8(uint8_t val)
{
    if(left() < sizeof(uint8_t))
        return false;
    write8(val, m_data, m_offset);
    m_offset += sizeof(uint8_t);
    return true;
}

bool BufferStream::writeBE16(uint16_t val)
{
    if(left() < sizeof(uint16_t))
        return false;
    writeBE16(val, m_data, m_offset);
    m_offset += sizeof(uint16_t);
    return true;
}

bool BufferStream::writeLE16(uint16_t val)
{
    if(left() < sizeof(uint16_t))
        return false;
    writeLE16(val, m_data, m_offset);
    m_offset += sizeof(uint16_t);
    return true;
}

bool BufferStream::writeBE32(uint32_t val)
{
    if(left() < sizeof(uint32_t))
        return false;
    writeBE32(val, m_data, m_offset);
    m_offset += sizeof(uint32_t);
    return true;
}

bool BufferStream::writeLE32(uint32_t val)
{
    if(left() < sizeof(uint32_t))
        return false;
    writeLE32(val, m_data, m_offset);
    m_offset += sizeof(uint32_t);
    return true;
}

bool BufferStream::writeFP32(float val)
{
    if(left() < sizeof(float))
        return false;
    uint32_t uval = *(uint32_t *)&val;
    writeLE32(uval, m_data, m_offset);
    m_offset += sizeof(float);
    return true;
}

bool BufferStream::putZExtBits(unsigned val, int offset, int count)
{
    Q_ASSERT((offset + count) <= 32);
    if(left() < sizeof(uint32_t))
        return false;
    uint64_t oldVal = readLE32(m_data, m_offset);
    uint64_t mask = ((1 << count) - 1);
    uint64_t shiftedBits = (val & mask) << offset;
    uint32_t newVal = (uint32_t)(oldVal | shiftedBits);
    writeLE32(newVal, m_data, m_offset);
    return true;
}

bool BufferStream::putSExtBits(int val, int offset, int count)
{
    // Saturate the input. Negative values could be larger than the number of bits to put.
    Q_ASSERT((offset + count) <= 32);
    int lowLimit = -(1 << count);
    val = qMax(lowLimit, val);
    
    // We can just cast the saturated value to unsigned as any extra bits on the
    // left of the sign bit will be masked out.
    return putZExtBits((unsigned)val, offset, count);
}

uint8_t BufferStream::read8(const uint8_t *buffer, uint32_t offset)
{
    return buffer[offset];
}

uint16_t BufferStream::readBE16(const uint8_t *buffer, uint32_t offset)
{
    uint16_t val = 0;
    val += (buffer[offset + 0] << 8);
    val += (buffer[offset + 1] << 0);
    return val;
}

uint16_t BufferStream::readLE16(const uint8_t *buffer, uint32_t offset)
{
    uint16_t val = 0;
    val += (buffer[offset + 0] << 0);
    val += (buffer[offset + 1] << 8);
    return val;
}

uint32_t BufferStream::readBE32(const uint8_t *buffer, uint32_t offset)
{
    uint32_t val = 0;
    val += (buffer[offset + 0] << 24);
    val += (buffer[offset + 1] << 16);
    val += (buffer[offset + 2] <<  8);
    val += (buffer[offset + 3] <<  0);
    return val;
}

uint32_t BufferStream::readLE32(const uint8_t *buffer, uint32_t offset)
{
    uint32_t val = 0;
    val += (buffer[offset + 0] <<  0);
    val += (buffer[offset + 1] <<  8);
    val += (buffer[offset + 2] << 16);
    val += (buffer[offset + 3] << 24);
    return val;
}

void BufferStream::write8(uint8_t val, uint8_t *buffer, uint32_t offset)
{
    buffer[offset] = val;
}

void BufferStream::writeBE16(uint16_t val, uint8_t *buffer, uint32_t offset)
{
    buffer[offset + 0] = (val >> 8) & 0xff;
    buffer[offset + 1] = (val >> 0) & 0xff;
}

void BufferStream::writeLE16(uint16_t val, uint8_t *buffer, uint32_t offset)
{
    buffer[offset + 0] = (val >> 0) & 0xff;
    buffer[offset + 1] = (val >> 8) & 0xff;
}

void BufferStream::writeBE32(uint32_t val, uint8_t *buffer, uint32_t offset)
{
    buffer[offset + 0] = (val >> 24) & 0xff;
    buffer[offset + 1] = (val >> 16) & 0xff;
    buffer[offset + 2] = (val >>  8) & 0xff;
    buffer[offset + 3] = (val >>  0) & 0xff;
}

void BufferStream::writeLE32(uint32_t val, uint8_t *buffer, uint32_t offset)
{
    buffer[offset + 0] = (val >>  0) & 0xff;
    buffer[offset + 1] = (val >>  8) & 0xff;
    buffer[offset + 2] = (val >> 16) & 0xff;
    buffer[offset + 3] = (val >> 24) & 0xff;
}

uint16_t BufferStream::swap16(uint16_t val)
{
    uint8_t temp[2];
    writeBE16(val, temp, 0);
    return readLE16(temp, 0);
}
