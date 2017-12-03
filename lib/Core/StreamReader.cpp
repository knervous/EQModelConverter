// Copyright (C) 2012 PiB <pixelbound@gmail.com>
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

#include <stdarg.h>
#include <QIODevice>
#include "EQuilibre/Core/StreamReader.h"

StreamReader::StreamReader(QIODevice *stream)
{
    m_stream = stream;
}

QIODevice *StreamReader::stream() const
{
    return m_stream;
}

bool StreamReader::unpackField(char type, void *field)
{
    switch(type)
    {
    case 'I':
        return readUint32((uint32_t *)field);
    case 'i':
        return readInt32((int32_t *)field);
    case 'H':
        return readUint16((uint16_t *)field);
    case 'h':
        return readInt16((int16_t *)field);
    case 'B':
        return readUint8((uint8_t *)field);
    case 'b':
        return readInt8((int8_t *)field);
    case 'f':
        return readFloat32((float *)field);
    default:
        return false;
    }
}

bool StreamReader::unpackFields(const char *types, ...)
{
    va_list args;
    va_start(args, types);
    while(*types)
    {
        void *dest = va_arg(args, void *);
        if(!unpackField(*types, dest))
            return false;
        types++;
    }
    va_end(args);
    return true;
}

bool StreamReader::unpackStruct(const char *types, void *first)
{
    uint8_t *dest = (uint8_t *)first;
    while(*types)
    {
        if(!unpackField(*types, dest))
            return false;
        dest += fieldSize(*types);
        types++;
    }
    return true;
}

bool StreamReader::unpackArray(const char *types, uint32_t count, void *first)
{
    uint8_t *dest = (uint8_t *)first;
    uint32_t size = structSize(types);
    for(uint32_t i = 0; i < count; i++)
    {
        if(!unpackStruct(types, dest))
            return false;
        dest += size;
    }
    return true;
}

uint32_t StreamReader::structSize(const char *types) const
{
    uint32_t s = 0;
    while(*types)
    {
        s += fieldSize(*types);
        types++;
    }
    return s;
}

bool StreamReader::readRaw(char *dest, size_t n)
{
    size_t left = n;
    while(left > 0)
    {
        qint64 read = m_stream->read(dest, left);
        if(read < 1)
            return false;
        left -= (size_t)read;
        dest += read;
    }
    return true;
}

bool StreamReader::readInt8(int8_t *dest)
{
    uint8_t data;
    if(!readRaw((char *)&data, 1))
        return false;
    *dest = (int8_t)data;
    return true;
}

bool StreamReader::readUint8(uint8_t *dest)
{
    uint8_t data;
    if(!readRaw((char *)&data, 1))
        return false;
    *dest = data;
    return true;
}

bool StreamReader::readInt16(int16_t *dest)
{
    uint8_t data[2];
    if(!readRaw((char *)&data, 2))
        return false;
    *dest = (data[1] << 8) | data[0];
    return true;
}

bool StreamReader::readUint16(uint16_t *dest)
{
    uint8_t data[2];
    if(!readRaw((char *)&data, 2))
        return false;
    *dest = (data[1] << 8) | data[0];
    return true;
}

bool StreamReader::readInt32(int32_t *dest)
{
    uint8_t data[4];
    if(!readRaw((char *)&data, 4))
        return false;
    *dest = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
    return true;
}

bool StreamReader::readUint32(uint32_t *dest)
{
    uint8_t data[4];
    if(!readRaw((char *)&data, 4))
        return false;
    *dest = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
    return true;
}

bool StreamReader::readFloat32(float *dest)
{
    uint8_t data[4];
    if(!readRaw((char *)&data, 4))
        return false;
    *dest = *((float *)data);
    return true;
}

uint32_t StreamReader::fieldSize(char c) const
{
    switch(c)
    {
    case 'I':
    case 'i':
    case 'f':
        return 4;
    case 'H':
    case 'h':
        return 2;
    case 'B':
    case 'b':
        return 1;
    default:
        return 0;
    }
}

bool StreamReader::readString(uint32_t size, QString *dest)
{
    QByteArray data = m_stream->read(size);
    if((uint32_t)data.length() < size)
        return false;
    *dest = QString(data);
    return true;
}
