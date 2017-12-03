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

#ifndef EQUILIBRE_GAME_ZONE_H
#define EQUILIBRE_GAME_ZONE_H

#include <vector>
#include <QObject>
#include <QVector>
//#include "Newton.h"
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/Geometry.h"
#include "EQuilibre/Core/World.h"
#include "EQuilibre/Game/GamePacks.h"

class Camera;
class Game;
struct GameUpdate;
class PFSArchive;
class WLDData;
class FrameStat;
class MaterialArray;
class MeshBuffer;
class RenderProgram;
class SoundTrigger;
struct SpawnInfo;
class Actor;
class CharacterActor;
class LightActor;
class WLDMesh;
class ZoneTerrain;
class ZoneObjects;
class ZoneActors;

/*!
  \brief Describes a zone of the world.
  */
class  Zone : public QObject
{
public:
    Zone(Game *game);
    virtual ~Zone();

    bool isLoaded() const;
    const ZoneInfo & info() const;
    uint16_t id() const;
    
    Game * game() const;
    CharacterActor * player() const;
    Camera * camera() const;
    ZoneTerrain * terrain() const;
    ZoneObjects * objects() const;
    ZoneActors * actors() const;
    const QVector<LightActor *> & lights() const;
    //NewtonWorld * collisionWorld();
    
    bool load(QString path, QString name);
    bool load(QString path, const ZoneInfo &info);

    void unload();
    void draw(RenderProgram *staticProg, RenderProgram *actorProg);
    void update(const GameUpdate &gu);
    
    void playerEntry();
    void playerExit();
    
    void playerJumped();
    
    void newCollisionCheck();
    
    void currentSoundTriggers(QVector<SoundTrigger *> &triggers) const;
    
signals:
    void loading();
    void loaded();
    void loadFailed(QString name);
    void unloading();
    void unloaded();
    void playerEntered();
    void playerLeft();

private:
    bool importLightSources(PFSArchive *archive);
    static void frustumCullingCallback(Actor *actor, void *user);

    Q_OBJECT
    Game *m_game;
    ZoneInfo m_info;
    ZoneTerrain *m_terrain;
    ZoneObjects *m_objects;
    ZoneActors *m_actors;
    PFSArchive *m_mainArchive;
    WLDData *m_mainWld;
    bool m_loaded;
    QVector<LightActor *> m_lights;
    QVector<SoundTrigger *> m_soundTriggers;
    Camera *m_camera;
    
    FrameStat *m_collisionChecksStat;
    int m_collisionChecks;
    //NewtonWorld *m_collisionWorld;
};

class  SkyDef
{
public:
    SkyDef();

    WLDMesh *mainLayer;
    WLDMesh *secondLayer;
};

/*!
  \brief Holds the resources needed to render a zone's sky dome.
  */
class  ZoneSky
{
public:
    ZoneSky(Game *game);
    virtual ~ZoneSky();
    void clear();
    AssetLoadState state() const;
    bool load(QString path);
    bool upload();
    void draw(RenderProgram *prog, Zone *zone);
    
private:
    Game *m_game;
    PFSArchive *m_skyArchive;
    WLDData *m_skyWld;
    MaterialArray *m_skyMaterials;
    std::vector<SkyDef> m_skyDefs;
    MeshBuffer *m_skyBuffer;
    AssetLoadState m_state;
};

#endif
