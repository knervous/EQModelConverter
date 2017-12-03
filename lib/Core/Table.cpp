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

#include "EQuilibre/Core/Table.h"

int findTableEntry(uint32_t entryID, const uint16_t *indexToID, int count)
{
    int low = 0;
    int high = count - 1;
    int current = 0;
    uint16_t currentID = 0;
    while(low <= high)
    {
        current = (low + high) / 2;
        currentID = indexToID[current];
        if(currentID == entryID)
            break;    
        else if(currentID < entryID)
            low = current + 1;
        else
            high = current - 1;
    }
    return (currentID == entryID) ? current : -1;
}
