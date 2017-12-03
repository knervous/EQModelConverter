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

#include <string>
#include <inttypes.h>
#include <string.h>
extern "C" {
#include "EQCryptoAPI.h"
}

static const uint8_t HEX_DIGITS[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

#define LOGIN_HEADER_SIZE 10

/** Round up the value to the next power of two. */
static unsigned int roundUp(unsigned int val, unsigned int alignPoT)
{
    return (val + (alignPoT - 1)) & ~(alignPoT - 1);
}

char* DecryptUsernamePassword(const char* buffer, unsigned int bufferSize, int mode)
{
    if(bufferSize < (LOGIN_HEADER_SIZE + 1))
        return NULL;
    
    // Extract the login and password hash from the plaintext.
    const int HASH_SIZE = 20;
    std::string login(buffer + LOGIN_HEADER_SIZE);
    std::string hash(buffer + LOGIN_HEADER_SIZE + login.size() + 1, HASH_SIZE);
    std::string hashHex;
    for(int i = 0; i < hash.size(); i++)
    {
        int hi = (hash[i] & 0xf0) >> 4;
        int lo = (hash[i] & 0x0f);
        hashHex.push_back(HEX_DIGITS[hi]);
        hashHex.push_back(HEX_DIGITS[lo]);
    }
    
    // Create a buffer to hold the credentials: hash followed by login.
    unsigned int credSize = login.size() + 1 + hashHex.size() + 1;
    char *credData = new char[credSize];
    memset(credData, 0, credSize);
    strncpy(credData, hashHex.c_str(), hashHex.size());
    strncpy(credData + hashHex.size() + 1, login.c_str(), login.size());
    return credData;
}

char* Encrypt(const char* buffer, unsigned int bufferSize, unsigned int &outSize)
{
    // Do not encrypt the buffer. Just copy it after padding it with zeros.
    outSize = roundUp(bufferSize, 8);
    char *credData = new char[outSize];
    memset(credData, 0, outSize);
    memcpy(credData, buffer, bufferSize);
    return credData;
}

void _HeapDeleteCharBuffer(char *buffer)
{
    delete [] buffer;
}
