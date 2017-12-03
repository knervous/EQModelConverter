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

#ifndef EQUILIBRE_CORE_BUFFER_STREAM_H
#define EQUILIBRE_CORE_BUFFER_STREAM_H

#include "EQuilibre/Core/Platform.h"

class  BufferStream
{
public:
    /** Create a new packet stream.
      */
    BufferStream(uint8_t *buffer, uint32_t size);
    
    uint32_t offset() const;
    void setOffset(uint32_t newOffset);
    
    uint32_t left() const;
    uint8_t * current() const;
    
    void skip(uint32_t bytes);
    
    char * readCString(uint32_t fixedSize=0);
    uint8_t read8();
    uint16_t readBE16();
    uint16_t readLE16();
    uint32_t readBE32();
    uint32_t readLE32();
    float readFP32();
    unsigned peekZExtBits(int offset, int count);
    int peekSExtBits(int offset, int count);
    
    bool writeCString(const char *str, uint32_t fixedSize=0);
    bool write8(uint8_t val);
    bool writeBE16(uint16_t val);
    bool writeLE16(uint16_t val);
    bool writeBE32(uint32_t val);
    bool writeLE32(uint32_t val);
    bool writeFP32(float val);
    bool putZExtBits(unsigned val, int offset, int count);
    bool putSExtBits(int val, int offset, int count);
    
    static uint8_t read8(const uint8_t *buffer, uint32_t offset);
    static uint16_t readBE16(const uint8_t *buffer, uint32_t offset);
    static uint16_t readLE16(const uint8_t *buffer, uint32_t offset);
    static uint32_t readBE32(const uint8_t *buffer, uint32_t offset);
    static uint32_t readLE32(const uint8_t *buffer, uint32_t offset);
    
    static void write8(uint8_t val, uint8_t *buffer, uint32_t offset);
    static void writeBE16(uint16_t val, uint8_t *buffer, uint32_t offset);
    static void writeLE16(uint16_t val, uint8_t *buffer, uint32_t offset);
    static void writeBE32(uint32_t val, uint8_t *buffer, uint32_t offset);
    static void writeLE32(uint32_t val, uint8_t *buffer, uint32_t offset);
    
    static uint16_t swap16(uint16_t val);
    
private:
    uint8_t *m_data;
    uint32_t m_size;
    uint32_t m_offset;
};

#endif
