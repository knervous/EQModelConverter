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

#ifndef EQUILIBRE_CORE_WORLD_H
#define EQUILIBRE_CORE_WORLD_H

#include <QAbstractItemModel>
#include <QString>
#include <QVector>
#include <QColor>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/LinearMath.h"

enum ChannelID
{
    eChannelGuild = 0,
    eChannelGroup = 2,
    eChannelShout = 3,
    eChannelAuction = 4,
    eChannelOOC = 5,
    eChannelBroadcast = 6,
    eChannelTell = 7,
    eChannelSay = 8,
    eChannelGM = 11,
    eChannelRaid = 15,
    eChannelUCS = 20,
    eChannelEmotes = 21
};

struct ServerInfo
{
    ServerInfo();

    QString host;
    QString name;
    QString locale1;
    QString locale2;
    uint32_t type;
    uint32_t runtimeID;
    uint32_t status;
    uint32_t numPlayers;

    QColor statusColor() const;
    const char * statusText() const;
};

/*!
  \brief Contains some rendering information about a zone.
  */
class ZoneInfo
{
public:
    uint16_t zoneID;
    uint16_t instanceID;
    QString name;
    QString longName;
    int skyID;
    vec3 safePos;
    float underworldZ;
    int flags;
    float minClip;
    float maxClip;
    float fogMinClip;
    float fogMaxClip;
    vec4 fogColor;
    float fogDensity;
    
    void clear();
    bool findByID(uint16_t zoneID, int *indexOut = NULL);
    bool findByIndex(int index);
    bool findByName(QString name);
    
    static uint16_t totalZones();
};

class ZonePoint
{
public:
    uint32_t pointID;
    vec3 destPos;
    float destHeading;
    uint16_t destZoneID;
    uint16_t destInstanceID;
};

enum ZoneRegionType
{
    eRegionNormal = 0,
    eRegionUnderwater = 1,
    eRegionLava = 2,
    eRegionPvP = 3,
    eRegionZonePoint = 4
};

struct ObjectInfo
{
    ObjectInfo();
    uint32_t objectID;
    uint32_t type;
    QString name;
    vec3 pos;
    float heading;
    uint16_t zoneID;
    uint16_t instanceID;
};

/** Possible status values in PlayResponse messages. */
enum PlayStatus
{
    PlayAllowed = 101,
    PlayDenied = 326,
    PlaySuspended = 337,
    PlayBanned = 338,
    PlayWordFull = 303
};

class ServerList : public QAbstractItemModel
{
public:
    ServerList();
    QVector<ServerInfo> & items();
    const QVector<ServerInfo> & items() const;
    size_t size() const;
    
    const ServerInfo *currentServer() const;
    int currentServerIndex() const;
    void setCurrentServerIndex(int newIndex);
    
    void append(const ServerInfo &info);
    void clear();
    
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    
private:
    const ServerInfo *toObject(QModelIndex index) const;
    
    QVector<ServerInfo> m_items;
    int m_currentIndex;
};

class ZoneList : public QAbstractItemModel
{
public:
    ZoneList();
    QVector<ZoneInfo> & items();
    const QVector<ZoneInfo> & items() const;
    size_t size() const;
    
    void append(const ZoneInfo &info);
    void clear();
    
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    
private:
    const ZoneInfo *toObject(QModelIndex index) const;
    
    QVector<ZoneInfo> m_items;
};

#endif
