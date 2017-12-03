// Copyright (C) 2012-2013 PiB <pixelbound@gmail.com>
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

#ifndef EQUILIBRE_GAME_ZONE_OBJECTS_H
#define EQUILIBRE_GAME_ZONE_OBJECTS_H

#include <QVector>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/Geometry.h"
#include "EQuilibre/Game/GamePacks.h"

class Game;
struct GameUpdate;
class ObjectPack;
struct ObjectInfo;
class Zone;
class PFSArchive;
class WLDData;
class ObjectActor;
class WLDMesh;
class OctreeIndex;
class FrameStat;
class RenderProgram;

/*!
  \brief Holds the resources needed to render a zone's static objects.
  */
class  ZoneObjects
{
public:
    ZoneObjects(Zone *zone);
    virtual ~ZoneObjects();
    
    AssetLoadState state() const;
    const AABox & bounds() const;
    AABox & bounds();
    QVector<ObjectActor *> & visibleObjects();

    bool load(QString path, QString name, PFSArchive *mainArchive);
    void addTo(OctreeIndex *tree);
    ObjectActor * createObject(const ObjectInfo &info);
    ObjectActor * createEquip(uint32_t modelID);
    
    void update(const GameUpdate &gu);
    bool upload();
    void draw(RenderProgram *prog);
    void unload();
    void resetVisible();

private:
    void importMeshes();
    void importActors();
    void draw(RenderProgram *prog, std::vector<ObjectActor *> &objects, ObjectPack *pack);
    
    Zone *m_zone;
    AABox m_bounds;
    AssetLoadState m_state;
    ObjectPack *m_pack;
    WLDData *m_objDefWld;
    QVector<ObjectActor *> m_objects;
    QVector<ObjectActor *> m_visibleObjects;
    FrameStat *m_objectsStat;
    FrameStat *m_objectsStatGPU;
    FrameStat *m_drawnObjectsStat;
};

#endif
