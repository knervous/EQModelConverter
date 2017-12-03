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

#ifndef EQUILIBRE_GAME_GAME_H
#define EQUILIBRE_GAME_GAME_H

#include <QString>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Core/Geometry.h"
#include "EQuilibre/Render/RenderContext.h"

class QElapsedTimer;
class QSettings;
class GamePacks;
class PFSArchive;
class Launcher;
class Log;
class GameClient;
class Zone;
class ZoneActors;
class ZoneList;
class ZoneSky;
struct SpawnInfo;
class Animation;
class CharacterActor;
class WLDMesh;
class WLDData;

enum GameFlags
{
    eGameNone = 0x0000,
    eGameShowTerrain = 0x0001,
    eGameShowObjects = 0x0002,
    eGameShowCharacters = 0x0004,
    eGameShowFog = 0x0008,
    eGameShowSky = 0x0010,
    eGameCullObjects = 0x0020,
    eGameShowSoundTriggers = 0x0040,
    eGameFrustumIsFrozen = 0x0080,
    eGameAllowMultiJumps = 0x0100,
    eGameApplyGravity = 0x00200,
    eGameApplyGravityNet = 0x00400,
    eGameDrawCapsule = 0x00800,
    eGameGPUSkinning = 0x01000,
    eGameLighting = 0x02000,
    // These flags are reset at the end of each frame.
    eGameFrameAction1 = 0x20000000,
    eGameFrameAction2 = 0x40000000,
    eGameFrameAction3 = 0x80000000
};

struct GameUpdate
{
    double currentTime;
    double sinceLastUpdate;
};

/*!
  \brief Contains the global state of the game.
  */
class Game : public QObject
{
public:
    Game(Log *log = 0, Launcher *launcher = 0);
    virtual ~Game();
    
    Log * log() const;
    void setLog(Log *newLog);
    
    QSettings *settings() const;
    
    GameFlags flags() const;
    bool hasFlag(GameFlags flag) const;
    void setFlag(GameFlags flag, bool set);
    void toggleFlag(GameFlags flag);
    
    const vec3 & gravity() const;
    
    float minDistanceToShowCharacter() const;
    
    void freezeFrustum();
    void unFreezeFrustum();
    
    QString assetPath() const;
    void setAssetPath(QString path);
    
    RenderContext * renderContext() const;
    GameClient * client() const;
    Zone * zone() const;
    ZoneSky * sky() const;
    GamePacks * packs() const;
    ZoneList * zones() const;
    
    bool loadSky(QString path);
    
    double currentTime() const;
    void update();
    
    void drawBuiltinObject(MeshData *object, RenderProgram *prog);
    
    void clear();
    void clearBuffer(MeshBuffer* &buffer);
    void clearFence(fence_t &fence);
    void clearMaterials(MaterialArray *array);

    
    static const int MOVEMENT_TICKS_PER_SEC = 60;
    static const float NET_VELOCITY_RATIO;
    static const float PLAYER_VELOCITY_MOD;

private:
    void updateZones();
    
    Q_OBJECT
    GameClient *m_client;
    GamePacks *m_packs;
    ZoneList *m_zones;
    Zone *m_zone;
    ZoneSky *m_sky;
    RenderContext *m_renderCtx;
    GameFlags m_flags;
    vec3 m_gravity;
    QElapsedTimer *m_gameTimer;
    double m_lastUpdateTime;
    FrameStat *m_updateStat;
    float m_minDistanceToShowCharacter;
    QSettings *m_settings;
    Log *m_log;
};

class Camera
{
public:
    Camera(Game *game);
    
    Game * game() const;
    
    CharacterActor * charActor() const;
    void setCharActor(CharacterActor *newActor);
    
    float cameraDistance() const;
    void setCameraDistance(float dist);
    
    /** Angle describing where the character is looking at, up or down. */
    float lookPitch() const;
    void setLookPitch(float newPitch);
    
    
    vec3 lookOrient() const;
    
    Frustum & frustum();
    Frustum & frozenFrustum();
    Frustum & realFrustum();
    
    void calculateViewFrustum();
    void calculateViewFrustum(Frustum &frustum) const;
    void freezeFrustum();
    
    matrix4 movementMatrix() const;
    
private:
    Game *m_game;
    CharacterActor *m_charActor;
    float m_cameraDistance;
    float m_lookPitch;
    Frustum m_frustum;
    Frustum m_frozenFrustum;
};

#endif
