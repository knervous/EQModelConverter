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

#include <tomcrypt.h>
#include "EQuilibre/Core/Authentication.h"
#include "EQuilibre/Core/BufferStream.h"
#include "EQuilibre/Core/MessageStructs.h"

static void SHA1(const std::string &input, std::string &output)
{
    const int HASH_SIZE = 20;
    hash_state md;
    sha1_init(&md);
    sha1_process(&md, (const uint8_t *)input.c_str(), input.size());
    output.clear();
    output.resize(HASH_SIZE, '\0');
    sha1_done(&md, (uint8_t *)&*output.begin());
}

int Authentication::loginRequestSize(const LoginMessage &msg)
{
    // Figure out how large the packet will be.
    // Allowed packet lengths: 20, 28, 36, 44, etc.
    const int hashSize = 20;
    int packetSize = (hashSize + msg.user.size() + 14);
    int allowedSize = 20;
    while(allowedSize < packetSize)
        allowedSize += 8;
    return allowedSize;  
}

bool Authentication::encodeLoginRequest(const LoginMessage &msg, BufferStream &s)
{
    std::string hash;
    SHA1(msg.password.toStdString(), hash);
    
    s.writeLE32(msg.unknownA);
    s.writeLE32(msg.unknownB);
    s.writeLE16(msg.unknownC);
    s.writeCString(msg.user.toUtf8().constData());
    s.writeCString(hash.c_str());
    return true;
}

bool Authentication::decodeLoginResponse(LoginAcceptedMessage &msg, BufferStream &s)
{
    if(s.left() < 44)
        return false;
    msg.unknownA = s.readLE32();
    msg.unknownB = s.readLE32();
    msg.unknownC = s.readLE16();
    msg.status = s.readLE32();
    msg.unknownD = s.readLE32();
    msg.userID = s.readLE32();
    const char *keyStr = s.readCString(16);
    if(!keyStr || (s.left() < 4))
        return false;
    msg.key = QString(keyStr);
    msg.failedAttempts = s.readLE32();
    return true;
}
