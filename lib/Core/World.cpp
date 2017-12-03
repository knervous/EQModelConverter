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

#include <QFile>
#include <QTextStream>
#include "EQuilibre/Core/World.h"
#include "EQuilibre/Core/Table.h"
#include "EQuilibre/Core/Zones.def"

// This array contains all zone identifiers in increasing order.
const static uint16_t IdentifierByIndex[ZONE_COUNT] = {
#define ZONE_ENTRY(id, name, long_name, sky, safeY, safeX, safeZ, underworldZ, \
                   minClip, maxClip, fogMinClip, fogMaxClip, fogR, fogG, fogB, fogDensity) \
    id,
#include "EQuilibre/Core/Zones.def"
#undef ZONE_ENTRY
};

// This array contains zone info with the identifier matching the index.
const static ZoneInfo Zones[ZONE_COUNT] = {
#define ZONE_ENTRY(id, name, long_name, sky, safeY, safeX, safeZ, underworldZ, \
                   minClip, maxClip, fogMinClip, fogMaxClip, fogR, fogG, fogB, fogDensity) \
    {id, 0, name, long_name, sky, vec3(safeX, safeY, safeZ), underworldZ, 0, \
     minClip, maxClip, fogMinClip, fogMaxClip, \
     vec4(fogR / 255.0f, fogG / 255.0f, fogB / 255.0f, 1.0f), fogDensity},
#include "EQuilibre/Core/Zones.def"
#undef ZONE_ENTRY
};

ServerInfo::ServerInfo()
{
    type = 0;
    runtimeID = 0;
    status = 0;
    numPlayers = 0;
}

QColor ServerInfo::statusColor() const
{
    switch(status)
    {
    default:
    case 0:
        return QColor(Qt::black);
    case 1:
        return QColor(Qt::red);
    case 2:
        return QColor(Qt::green);
    }
}

const char * ServerInfo::statusText() const
{
    switch(status)
    {
    default:
    case 0:
        return "Unknown";
    case 1:
        return "Down";
    case 2:
        return "Up";
    }
}

////////////////////////////////////////////////////////////////////////////////

ServerList::ServerList() : QAbstractItemModel()
{
    m_currentIndex = -1;
}

QVector<ServerInfo> & ServerList::items()
{
    return m_items;
}

const QVector<ServerInfo> & ServerList::items() const
{
    return m_items;
}

size_t ServerList::size() const
{
    return m_items.size();
}

const ServerInfo * ServerList::currentServer() const
{
    if((m_currentIndex < 0) || (m_currentIndex >= m_items.size()))
        return NULL;
    return &m_items[m_currentIndex];
}

int ServerList::currentServerIndex() const
{
    return m_currentIndex;
}

void ServerList::setCurrentServerIndex(int newIndex)
{
    m_currentIndex = newIndex;
}

void ServerList::append(const ServerInfo &info)
{
    int index = m_items.count();
    beginInsertRows(QModelIndex(), index, index);
    m_items.append(info);
    endInsertRows();
}

void ServerList::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

const ServerInfo * ServerList::toObject(QModelIndex index) const
{
    if(!index.isValid() || (index.row() < 0) || (index.row() >= rowCount()))
        return NULL;
    else
        return &m_items[index.row()];
}

QModelIndex ServerList::index(int row, int column, const QModelIndex &parent) const
{
    if(parent.isValid() || (row < 0) || (row >= rowCount()))
        return QModelIndex();
    else
        return createIndex(row, column);
}

QModelIndex ServerList::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int ServerList::rowCount(const QModelIndex &parent) const
{
    return m_items.count();
}

int ServerList::columnCount(const QModelIndex &parent) const
{
    return 4;
}

QVariant ServerList::data(const QModelIndex &index, int role) const
{
    const ServerInfo *server = toObject(index);
    if(!server)
        return QVariant();
    else if((role == Qt::DisplayRole) || (role == Qt::ToolTipRole))
    {
        switch(index.column())
        {
        case 0:
            return server->name;
        case 1:
            return server->host;
        case 2:
            return server->numPlayers;
        case 3:
            return server->statusText();
        }
    }
    else if(role == Qt::ForegroundRole)
    {
        return server->statusColor();
    }
    return QVariant();
}

QVariant ServerList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if((role == Qt::DisplayRole) && (orientation == Qt::Horizontal))
    {
        switch(section)
        {
        case 0:
            return "Name";
        case 1:
            return "Host";
        case 2:
            return "Players";
        case 3:
            return "Status";
        }
    }
    return QVariant();
}

Qt::ItemFlags ServerList::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

////////////////////////////////////////////////////////////////////////////////

void ZoneInfo::clear()
{
    zoneID = 0;
    instanceID = 0;
    name = QString();
    longName = QString();
    skyID = 0;
    fogColor = vec4(0.4f, 0.4f, 0.6f, 0.1f);
    fogMinClip = minClip = 450.0f;
    fogMaxClip = maxClip = 450.0f;
    fogDensity = 0.0f;
    underworldZ = -1000.0f;
    flags = 0;
}

uint16_t ZoneInfo::totalZones()
{
    return ZONE_COUNT;
}

bool ZoneInfo::findByID(uint16_t zoneID, int *indexOut)
{
    int index = findTableEntry(zoneID, IdentifierByIndex, ZONE_COUNT);
    if(index < 0)
    {
        if(indexOut)
            *indexOut = -1;
        return false;
    }
    *this = Zones[index];
    if(indexOut)
        *indexOut = index;
    return true;
}

bool ZoneInfo::findByIndex(int index)
{
    if((index < 0) || (index > ZONE_COUNT))
        return false;
    *this = Zones[index];
    return true;
}

bool ZoneInfo::findByName(QString name)
{
    for(uint16_t i = 0; i < ZONE_COUNT; i++)
    {
        if(Zones[i].name == name)
        {
            *this = Zones[i];
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

ZoneList::ZoneList() : QAbstractItemModel()
{
}

QVector<ZoneInfo> & ZoneList::items()
{
    return m_items;
}

const QVector<ZoneInfo> & ZoneList::items() const
{
    return m_items;
}

size_t ZoneList::size() const
{
    return m_items.size();
}

void ZoneList::append(const ZoneInfo &info)
{
    int index = m_items.count();
    beginInsertRows(QModelIndex(), index, index);
    m_items.append(info);
    endInsertRows();
}

void ZoneList::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

const ZoneInfo * ZoneList::toObject(QModelIndex index) const
{
    if(!index.isValid() || (index.row() < 0) || (index.row() >= rowCount()))
        return NULL;
    else
        return &m_items[index.row()];
}

QModelIndex ZoneList::index(int row, int column, const QModelIndex &parent) const
{
    if(parent.isValid() || (row < 0) || (row >= rowCount()))
        return QModelIndex();
    else
        return createIndex(row, column);
}

QModelIndex ZoneList::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int ZoneList::rowCount(const QModelIndex &parent) const
{
    return m_items.count();
}

int ZoneList::columnCount(const QModelIndex &parent) const
{
    return 4;
}

QVariant ZoneList::data(const QModelIndex &index, int role) const
{
    const ZoneInfo *zone = toObject(index);
    if(!zone)
        return QVariant();
    else if((role == Qt::DisplayRole) || (role == Qt::ToolTipRole))
    {
        switch(index.column())
        {
        case 0:
            return zone->zoneID;
        case 1:
            return zone->name;
        case 2:
            return zone->longName;
        case 3:
            if(zone->zoneID)
                return QString("%1 (%2)").arg(zone->longName).arg(zone->name);
            else
                return "<Invalid zone>";
        }
    }
    return QVariant();
}

QVariant ZoneList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if((role == Qt::DisplayRole) && (orientation == Qt::Horizontal))
    {
        switch(section)
        {
        case 0:
            return "ID";
        case 1:
            return "File";
        case 2:
            return "Name";
        case 3:
            return "Display";
        }
    }
    return QVariant();
}

Qt::ItemFlags ZoneList::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

////////////////////////////////////////////////////////////////////////////////

ObjectInfo::ObjectInfo()
{
    objectID = 0;
    type = 0;
    pos;
    heading = 0.0f;
    zoneID = 0;
    instanceID = 0;
}
