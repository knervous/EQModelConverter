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
#include <zlib.h>
#include <string.h>
#include <QDebug>
#include "EQuilibre/Network/Datagram.h"
#include "EQuilibre/Network/Packet.h"
#include "EQuilibre/Core/BufferStream.h"

Datagram::Datagram(uint8_t *buffer, uint32_t size)
{
    this->m_buffer = buffer;
    this->m_bufferSize = size;
    clear();
}

uint8_t * Datagram::buffer() const
{
    return m_buffer;
}

uint32_t Datagram::bufferSize() const
{
    return m_bufferSize;
}

uint16_t Datagram::type() const
{
    return m_packetType;
}

void Datagram::setType(uint16_t newType)
{
    m_packetType = newType;
}

uint16_t Datagram::seqNum() const
{
    return m_packetSeqNum;
}

void Datagram::setSeqNum(uint16_t newSeqNum)
{
    m_packetSeqNum = newSeqNum;
}

bool Datagram::hasSeqNum() const
{
    return m_hasSeqNum;
}

void Datagram::setHasSeqNum(bool hasSeqNum)
{
    m_hasSeqNum = hasSeqNum;
}

uint32_t Datagram::offset() const
{
    return m_packetOffset;
}

void Datagram::setOffset(uint32_t newOffset)
{
    m_packetOffset = newOffset;
}

uint32_t Datagram::size() const
{
    return m_packetSize;
}

void Datagram::setSize(uint32_t newSize)
{
    m_packetSize = newSize;
}

void Datagram::clear()
{
    m_packetType = m_packetSeqNum = 0;
    m_packetOffset = m_packetSize = 0;
    m_typeHi = m_typeLo = m_checksumSize = 0;
    m_compressed = m_rawMessage = m_hasSeqNum = false;
}

void Datagram::setBuffer(uint8_t *buffer, uint32_t size)
{
    clear();
    m_buffer = buffer;
    m_bufferSize = size;
}

bool Datagram::read(uint32_t crcKey, bool compressedSession)
{
    // Get the packet's ID.
    BufferStream streamIn(m_buffer, m_packetSize);
    bool success = readID(streamIn, false);
    
    // Get the packet's checksum and verify that it is correct.
    success = success && verifyChecksum(streamIn, crcKey);
    
    // Determine whether the packet really is compressed.
    success = success && (!compressedSession || readCompressedHeader(streamIn));

    // Determine the offset and size of the packet. Decompress it, if needed.
    success = success && readBody(streamIn, compressedSession);
    
    // Remove the sequence number from the packet.
    success = success && readSeqNum();
    
    //logMessage("read packet of type %d and size %d", packetType, packetSize);
    return success;
}

bool Datagram::readUnwrapped()
{
    // Get the packet's ID.
    BufferStream streamIn(m_buffer, m_packetSize);
    bool success = readID(streamIn, true);
    success = success && readBody(streamIn, false);
    success = success && readSeqNum();
    return success;
}

bool Datagram::readID(BufferStream &streamIn, bool unwrapped)
{
    if(streamIn.left() < 2)
        return false;
    m_packetType = streamIn.readBE16();
    if(!unwrapped)
    {
        m_rawMessage = (m_packetType > 0xff);
        if(m_rawMessage || hasChecksum(m_packetType))
            m_checksumSize = 2;
    }
    return true;
}

bool Datagram::verifyChecksum(BufferStream &streamIn, uint32_t crcKey)
{
    if(!m_checksumSize)
        return true;
    uint16_t checksumOffset = (m_packetSize - m_checksumSize);
    streamIn.setOffset(checksumOffset);
    uint16_t packetChecksum = streamIn.readBE16();
    uint16_t actualChecksum = calculateChecksum(m_buffer, checksumOffset, crcKey);
    if(actualChecksum != packetChecksum)
    {
        qDebug("packet checksum %x does not match computed checksum %x", 
               packetChecksum, actualChecksum);
        return false;
    }
    streamIn.setOffset(m_checksumSize);
    return true;
}

bool Datagram::readCompressedHeader(BufferStream &streamIn)
{
    uint8_t compressionFlag = 0;
    if(m_rawMessage)
    {
        // Handle raw messages differently.
        streamIn.setOffset(0);
        m_typeHi = streamIn.read8();
        compressionFlag = streamIn.read8();
        if(compressionFlag == 'Z')
        {
            // Deflate-compressed. The lower part of the message ID is
            // actually the first byte of the compressed payload.
            m_compressed = true;
        }
        else if(compressionFlag == 0xa5)
        {
            // Not compressed. The lower part of the message ID follows.
            m_typeLo = streamIn.read8();
            m_compressed = false;
        }
        else
        {
            qDebug("invalid compression flag: 0x%x", compressionFlag);
            return false;
        }
    }
    else
    {
        // Check the flag byte to see if the packet really is compressed.
        compressionFlag = streamIn.read8();
        if(compressionFlag == 'Z')
        {
            // Deflate-compressed.
            m_compressed = true;
        }
        else if(compressionFlag == 0xa5)
        {
            // Not compressed.
            m_compressed = false;
        }
        else
        {
            qDebug("invalid compression flag: 0x%x", compressionFlag);
            return false;
        }
    }
    return true;
}

bool Datagram::readBody(BufferStream &streamIn, bool compressedSession)
{
    // If the session is not compressed, we only need to set the packet offset and size.
    uint32_t bodySizeIn = (streamIn.left() - m_checksumSize);
    uint32_t bodyOffsetOut = streamIn.offset();
    uint32_t bodySizeOut = bodySizeIn;
    if(!compressedSession)
    {
        m_packetOffset = bodyOffsetOut;
        m_packetSize = bodySizeOut;
        return true;
    }
    
    if(m_compressed)
    {
        // Decompress the packet after the compressed data.
        bodyOffsetOut = roundUp(m_packetSize, 16);
        bodySizeOut = m_bufferSize - bodyOffsetOut;
        uint8_t *bodyOut = m_buffer + bodyOffsetOut;
        if(!decompress(streamIn.current(), bodySizeIn, bodyOut, bodySizeOut))
        {
            qDebug("could not decompress packet");
            return false;
        }
    }
    m_packetOffset = bodyOffsetOut;
    m_packetSize = bodySizeOut;
    
    // Reconstruct the ID of the raw message, which will need swapping.
    if(m_rawMessage)
    {
        if(m_compressed)
        {
            m_typeLo = m_buffer[m_packetOffset];
            m_packetOffset++;
            m_packetSize--;
        }
        m_packetType = ((m_typeHi << 8) | m_typeLo);
    }
    return true;
}

bool Datagram::readSeqNum()
{
    m_packetSeqNum = 0;
    m_hasSeqNum = false;
    if(hasSeqNum(m_packetType) && !m_rawMessage)
    {
        m_packetSeqNum = BufferStream::readBE16(m_buffer, m_packetOffset);
        m_packetOffset += sizeof(uint16_t);
        m_packetSize -= sizeof(uint16_t);
        m_hasSeqNum = true;
    }
    return true;
}

bool Datagram::write(BufferStream &inStream, uint32_t crcKey, bool compressedSession)
{
    // The header of compressed packets is one byte larger than normal.
    // This forces us to copy the contents of the packet to a buffer.
    // A buffer is needed anyway for compressed packets (common case).
    bool compressPacket = false;
    BufferStream outStream(m_buffer, m_bufferSize);
    outStream.writeBE16(m_packetType);
    if(compressedSession)
    {
        compressPacket = (inStream.left() > 10);
        outStream.write8(compressPacket ? 'Z' : 0xa5);
    }

    // Compress the message if needed or simply copy the payload to the buffer.
    uint32_t payloadSize = 0;
    if(compressPacket)
    {
        payloadSize = outStream.left();
        compress(inStream.current(), inStream.left(),
                 outStream.current(), payloadSize);
    }
    else
    {
        if(m_hasSeqNum)
            outStream.writeBE16(m_packetSeqNum);
        payloadSize = inStream.left();
        memcpy(outStream.current(), inStream.current(), payloadSize);
    }
    outStream.skip(payloadSize);

    // Compute the checksum if needed.
    if(hasChecksum(m_packetType))
    {
        uint16_t checksum = calculateChecksum(m_buffer, outStream.offset(), crcKey);
        outStream.writeBE16(checksum);
    }
    m_packetSize = outStream.offset();
    return true;
}

bool Datagram::hasChecksum(uint16_t type)
{
    switch((PacketType)type)
    {
    default:
        return true;
    case SM_SessionRequest:
    case SM_SessionResponse:
        return false;
    }
}

bool Datagram::hasSeqNum(uint16_t type)
{
    switch((PacketType)type)
    {
    default:
        return false;
    case SM_ApplicationPacket:
    case SM_Fragment:
        return true;
    }
}

uint16_t Datagram::calculateChecksum(uint8_t *data, uint32_t size, uint32_t crcKey)
{
    uint32_t crc = 0;
    crc = crc32(crc, (uint8_t *)&crcKey, sizeof(uint32_t));
    crc = crc32(crc, data, size);
    return (uint16_t)(crc & 0xffff);
}

void Datagram::compress(uint8_t *src, uint32_t srcSize,
                        uint8_t *dst, uint32_t &dstSize)
{
    z_stream zs;
    int status;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    deflateInit(&zs, Z_DEFAULT_COMPRESSION);

    // Compress the sequence number if there is one.
    zs.next_out = dst;
    zs.avail_out = dstSize;
    if(m_hasSeqNum)
    {
        uint8_t seqData[2];
        BufferStream::writeBE16(m_packetSeqNum, seqData, 0);
        zs.next_in = seqData;
        zs.avail_in = 2;
        status = deflate(&zs, Z_NO_FLUSH);
    }

    // Compress the payload.
    zs.next_in = src;
    zs.avail_in = srcSize;
    status = deflate(&zs, Z_FINISH);
    if(status != Z_STREAM_END)
    {
        dstSize = 0;
        return;
    }
    deflateEnd(&zs);
    dstSize = zs.total_out;
}

bool Datagram::decompress(uint8_t *src, uint32_t srcSize,
                          uint8_t *dst, uint32_t &dstSize)
{
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.next_in = src;
    zs.avail_in = srcSize;
    zs.next_out = dst;
    zs.avail_out = dstSize;

    inflateInit(&zs);
    int status = inflate(&zs, Z_FINISH);
    if(status != Z_STREAM_END)
    {
        dstSize = 0;
        return false;
    }
    inflateEnd(&zs);
    dstSize = zs.total_out;
    return true;
}
