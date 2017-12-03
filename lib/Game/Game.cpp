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

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QScopedPointer>
#include <QSettings>
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/CharacterActor.h"
#include "EQuilibre/Game/GamePacks.h"
#include "EQuilibre/Core/Fragments.h"
#include "EQuilibre/Core/Log.h"
#include "EQuilibre/Core/PFSArchive.h"
#include "EQuilibre/Core/StreamReader.h"
#include "EQuilibre/Core/WLDData.h"
#include "EQuilibre/Core/World.h"
//#include "EQuilibre/Game/GameClient.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/ZoneActors.h"
//#include "EQuilibre/Network/Launcher.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"

// Multiplying a 'network' speed by this gives the corresponding 'world' velocity.
const float Game::NET_VELOCITY_RATIO = 9.5f;

// PCs have NET_VELOCITY_FACTOR multiplied by this value.
const float Game::PLAYER_VELOCITY_MOD = 2.0f;

Game::Game(Log *log, Launcher *launcher)
{
    m_log = log;
    m_client = NULL;
    m_sky = NULL;
    m_flags = eGameNone;
    setFlag(eGameShowTerrain, true);
    setFlag(eGameShowObjects, true);
    setFlag(eGameShowCharacters, true);
    setFlag(eGameShowSky, true);
    setFlag(eGameShowFog, true);
    setFlag(eGameCullObjects, true);
    setFlag(eGameApplyGravity, true);
    setFlag(eGameGPUSkinning, true);
    m_gravity = vec3(0.0, 0.0, -1.0);
    m_updateStat = NULL;
    m_minDistanceToShowCharacter = 1.0;
    m_lastUpdateTime = 0.0;
    
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
        "EQuilibre", QString());
    m_gameTimer = new QElapsedTimer();
    m_gameTimer->start();
    m_renderCtx = new RenderContext();
    m_zones = new ZoneList();
    updateZones();
    m_packs = new GamePacks(this);
    m_zone = new Zone(this);
}

Game::~Game()
{
    clear();
    delete m_client;
    delete m_zone;
    delete m_packs;
    delete m_zones;
    delete m_renderCtx;
    delete m_gameTimer;
    delete m_settings;
}

void Game::clear()
{
    m_zone->unload();
    delete m_sky;
    m_sky = NULL;
    m_packs->clear();

}

void Game::clearBuffer(MeshBuffer* &buffer)
{
    if(buffer)
    {
        buffer->clear(m_renderCtx);
        delete buffer;
        buffer = NULL;
    }
}

void Game::clearFence(fence_t &fence)
{
    if(fence)
    {
        m_renderCtx->deleteFence(fence);
        fence = NULL;
    }
}

void Game::clearMaterials(MaterialArray *array)
{
    if(array)
    {
        array->clear(m_renderCtx);
    }
}

Log * Game::log() const
{
    return m_log;    
}

void Game::setLog(Log *newLog)
{
    m_log = newLog;
}

QSettings * Game::settings() const
{
    return m_settings;
}

GameFlags Game::flags() const
{
    return m_flags;
}

bool Game::hasFlag(GameFlags flag) const
{
    return m_flags & flag;
}

void Game::setFlag(GameFlags flag, bool set)
{
    if(set)
        m_flags = (GameFlags)(m_flags | flag);
    else
        m_flags = (GameFlags)(m_flags & ~flag);
}

void Game::toggleFlag(GameFlags flag)
{
    setFlag(flag, !hasFlag(flag));
}

void Game::freezeFrustum()
{
    if(!hasFlag(eGameFrustumIsFrozen))
    {
        m_zone->camera()->freezeFrustum();
        setFlag(eGameFrustumIsFrozen, true);
    }
}

void Game::unFreezeFrustum()
{
    setFlag(eGameFrustumIsFrozen, false);
}

const vec3 & Game::gravity() const
{
    return m_gravity;
}

QString Game::assetPath() const
{
    return m_settings->value("assetPath").toString();
}

void Game::setAssetPath(QString path)
{
     m_settings->setValue("assetPath", path);
     updateZones();
}

RenderContext * Game::renderContext() const
{
    return m_renderCtx;
}

GameClient * Game::client() const
{
    return m_client;
}

Zone * Game::zone() const
{
    return m_zone;
}

ZoneSky * Game::sky() const
{
    return m_sky;
}

float Game::minDistanceToShowCharacter() const
{
    return m_minDistanceToShowCharacter;
}

GamePacks * Game::packs() const
{
    return m_packs;
}

ZoneList * Game::zones() const
{
    return m_zones;
}

bool Game::loadSky(QString path)
{
    QScopedPointer<ZoneSky> sky(new ZoneSky(this));
    if(!sky->load(path))
        return false;
    m_sky = sky.take();
    return true;
}

void Game::drawBuiltinObject(MeshData *object, RenderProgram *prog)
{
    if(!object || !prog || !m_renderCtx->isValid())
    {
        return;
    }
    
    MeshBuffer *builtinBuffer = m_packs->builtinOjectBuffer();
    if(builtinBuffer->vertexBuffer == 0)
        builtinBuffer->upload(m_renderCtx);
    builtinBuffer->matGroups.clear();
    builtinBuffer->addMaterialGroups(object);
    
    RenderBatch batch(builtinBuffer, m_packs->builtinMaterials());
    prog->drawMesh(batch);
}

double Game::currentTime() const
{
    return (double)m_gameTimer->msecsSinceReference() * 0.000000001;
}

void Game::update()
{
    GameUpdate gu;
    gu.currentTime = currentTime();
    gu.sinceLastUpdate = gu.currentTime - m_lastUpdateTime;
    m_packs->update(gu);
    if(m_zone)
        m_zone->update(gu);
    m_lastUpdateTime = gu.currentTime;
}

static bool zoneLessThan(const ZoneInfo &a, const ZoneInfo &b)
{
    return a.longName < b.longName;
}

void Game::updateZones()
{
    QVector<ZoneInfo> newZones;
    ZoneInfo invalidZone;
    invalidZone.clear();
    newZones.append(invalidZone);
    
    QDir assetDir(assetPath());
    if(assetDir.exists())
    {
        for(unsigned i = 0; i < ZoneInfo::totalZones(); i++)
        {
            ZoneInfo zone;
            if(zone.findByIndex(i))
            {
                QString zoneFile = QString("%1.s3d").arg(zone.name);
                if(assetDir.exists(zoneFile))
                {
                    if(zone.longName.startsWith("The "))
                    {
                        zone.longName = zone.longName.mid(4);
                    }
                    newZones.append(zone);
                }
            }
        }
    }
    qSort(newZones.begin(), newZones.end(), &zoneLessThan);
    
    m_zones->clear();
    foreach(ZoneInfo zone, newZones)
    {
        m_zones->append(zone);
    }
}

////////////////////////////////////////////////////////////////////////////////

Camera::Camera(Game *game)
{
    m_game = game;
    m_charActor = NULL;
    m_cameraDistance = 0.0f;
    m_lookPitch = 0.0f;
}

Game * Camera::game() const
{
    return m_game;
}

CharacterActor * Camera::charActor() const
{
    return m_charActor;
}

void Camera::setCharActor(CharacterActor *newActor)
{
    m_charActor = newActor;
}

float Camera::lookPitch() const
{
    return m_lookPitch;   
}

void Camera::setLookPitch(float newPitch)
{
    m_lookPitch = newPitch;
}

vec3 Camera::lookOrient() const
{
    float heading = m_charActor ? m_charActor->heading() : 0.0f;
    return vec3(0.0f, m_lookPitch, heading);
}

Frustum & Camera::frustum()
{
    return m_frustum;
}

Frustum & Camera::frozenFrustum()
{
    return m_frozenFrustum;
}

Frustum & Camera::realFrustum()
{
    return m_game->hasFlag(eGameFrustumIsFrozen) ? m_frozenFrustum : m_frustum;
}

float Camera::cameraDistance() const
{
    return m_cameraDistance;
}

void Camera::setCameraDistance(float dist)
{
    m_cameraDistance = dist;
}

void Camera::calculateViewFrustum()
{
    calculateViewFrustum(m_frustum);
}

void Camera::calculateViewFrustum(Frustum &frustum) const
{
    if(!m_charActor)
    {
        return;
    }
    
    vec3 rot = lookOrient();
    matrix4 viewMat = matrix4::rotate(rot.z, 0.0f, 0.0f, 1.0f) *
                      matrix4::rotate(-rot.y, 0.0f, 1.0f, 0.0f) *
                      matrix4::rotate(rot.x, 1.0f, 0.0f, 0.0f);
    vec3 camPos(-m_cameraDistance, 0.0f, 0.0f);
    const float eyeLevel = 0.8f;
    vec3 eyePos(0.0f, 0.0f, m_charActor->capsuleHeight() * 0.5f * eyeLevel);
    vec3 eye = m_charActor->location() + eyePos + viewMat.map(camPos);
    frustum.setEye(eye);
    frustum.setFocus(eye + viewMat.map(vec3(1.0f, 0.0f, 0.0f)));
    frustum.setUp(vec3(0.0f, 0.0f, 1.0f));
    frustum.update();
}

void Camera::freezeFrustum()
{
    m_frozenFrustum = m_frustum;
    calculateViewFrustum(m_frozenFrustum);
}

matrix4 Camera::movementMatrix() const
{
    bool ghost = (m_cameraDistance < m_game->minDistanceToShowCharacter());
    ghost &= m_game->hasFlag(eGameAllowMultiJumps);
    matrix4 m;
    if(ghost)
        m = matrix4::rotate(lookOrient().y, 0.0, 1.0, 0.0);
    else
        m.setIdentity();
    m = m * matrix4::rotate(lookOrient().z, 0.0, 0.0, 1.0);
    return m;
}
