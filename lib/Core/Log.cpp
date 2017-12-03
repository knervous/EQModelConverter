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

#include <stdio.h>
#include <QDebug>
#include "EQuilibre/Core/Log.h"

Log::Log()
{
}

const QVector<QString> Log::lines() const
{
    return m_lines;
}

void Log::writeLine(QString text)
{
    m_lines.append(text);
    qDebug("%s", text.toLatin1().constData());
    emit logEntryAdded(text);
}

void Log::writeLine(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    writeLine(format, args);
    va_end(args);
}

void Log::writeLine(const char *format, va_list args)
{
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    writeLine(QString(buffer));
}
