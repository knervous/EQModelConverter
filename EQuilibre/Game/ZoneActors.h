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

#ifndef EQUILIBRE_GAME_ZONE_ACTORS_H
#define EQUILIBRE_GAME_ZONE_ACTORS_H

#include <vector>
#include <QVector>
#include <QList>
#include <QMap>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/Geometry.h"

class Game;
struct GameUpdate;
class Zone;
class CharacterActor;
class CharacterModel;
class CharacterPack;
class MaterialMap;
class OctreeIndex;
class FrameStat;
class RenderProgram;
struct SpawnInfo;

typedef std::vector<CharacterActor *> CharActorList;

/*!
  \brief Holds a list of actors present in a zone.
  */
class  ZoneActors
{
public:
    ZoneActors(Zone *zone);
    ~ZoneActors();
    
    QList<CharacterPack *> characterPacks() const;
    const QMap<uint32_t, CharacterActor *> & actors() const;
    std::vector<CharacterActor *> & visibleActors();
    OctreeIndex * index() const;
    
    CharacterActor * player() const;
    Zone * zone() const;
    
    CharacterPack * loadCharacters(QString archivePath, QString wldName = QString::null);
    OctreeIndex * createIndex(const AABox &bounds);
    CharacterActor * createActor(const SpawnInfo &info);
    CharacterActor * findActor(uint32_t spawnID);
    bool removeActor(uint32_t spawnID);
    
    void spawnPlayer(const SpawnInfo &info);
    void playerEntry();
    void playerExit();
    
    void update(const GameUpdate &gu);
    void updateVisible(const GameUpdate &gu);
    void draw(RenderProgram *prog);
    void unload();
    void unloadActors();
    void resetVisible();
    
private:
    void drawModel(RenderProgram *prog, CharacterModel *model);
    void drawModel(RenderProgram *prog, CharacterModel *model,
                   uint32_t drawMask, uint32_t &drawnMask);
    
    Game *m_game;
    Zone *m_zone;
    CharacterActor *m_player;
    
    // Duration between the newest movement tick and the current frame.
    double m_movementAheadTime;
    QList<CharacterPack *> m_charPacks;
    QMap<uint32_t, CharacterActor *> m_actors;
    CharActorList m_visibleActors;
    OctreeIndex *m_actorTree;
    FrameStat *m_actorsStat;
    FrameStat *m_drawnActorsStat;
    
    // Hold per-instance data when rendering character batches.
    CharActorList m_modelActors;
    CharActorList m_batchActors;
    std::vector<matrix4> m_batchMVMatrices;
    std::vector<MaterialMap *> m_batchMaterialMaps;
    std::vector<uint32_t> m_batchAnimIDs;
    std::vector<float> m_batchAnimFrames;
};

#endif
