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

#include <cmath>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include "EQuilibre/UI/CharacterScene.h"
#include "EQuilibre/Core/Character.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Game/CharacterActor.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/GamePacks.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/ZoneActors.h"

CharacterScene::CharacterScene(Game *game) : Scene(game->renderContext())
{
    m_sigma = 1.0;
    m_lastUpdateTime = 0.0f;
    m_game = game;
    m_camera = new Camera(game);
    m_selectedActorIndex = -1;
    m_player = game->zone()->player();
    m_skinningMode = HardwareSkinning;
    m_transState.last = vec3();
    m_rotState.last = vec3();
    m_transState.active = false;
    m_rotState.active = false;
    m_delta = vec3(-0.0, -0.0, -5.0);
    m_theta = vec3(-90.0, 00.0, 270.0);
    m_sigma = 0.5;
}

CharacterScene::~CharacterScene()
{
    delete m_camera;
}

Game * CharacterScene::game() const
{
    return m_game;
}

CharacterScene::SkinningMode CharacterScene::skinningMode() const
{
    return m_skinningMode;
}

void CharacterScene::setSkinningMode(SkinningMode newMode)
{
    m_skinningMode = newMode;
}

int CharacterScene::selectedActor() const
{
    return m_selectedActorIndex;
}

QString CharacterScene::selectedAnimName() const
{
    return m_animName;
}

void CharacterScene::setSelectedAnimName(QString newName)
{
    m_animName = newName;
    if(m_player->model())
    {
        CharacterAnimation animID = CharacterInfo::findAnimationByName(newName.toLatin1().constData());
        Animation *anim = m_player->findAnimation(newName);
        m_player->setAnimation(anim, animID);
    }
}

const QVector<ActorInfo> & CharacterScene::actors() const
{
    return m_actors;
}

CharacterActor * CharacterScene::actor() const
{
    return m_player;
}

void CharacterScene::setSelectedActor(int actorIndex)
{
    ActorInfo actor = m_actors.value(actorIndex);
    m_player->setPack(actor.pack);
    m_player->setModel(actor.model);
    setSelectedSkinID(0);
    setSelectedAnimName("POS");
    m_selectedActorIndex = actorIndex;
}

void CharacterScene::setSelectedSkinID(int skinID)
{
    for(int i = 0; i < eSlotCount; i++)
        m_player->setEquip((SlotID)i, skinID, 0xff000000);
}

void CharacterScene::init()
{
}

CharacterPack * CharacterScene::loadCharacters(QString archivePath)
{
    CharacterPack *charPack = m_game->packs()->loadCharacters(archivePath);
    if(charPack)
    {
        updateActorList();
        return charPack;
    }
    return NULL;
}

void CharacterScene::fillActorInfo(ActorInfo &info)
{
    RaceInfo raceInfo;
    info.raceID = info.model->race();
    info.genderID = info.model->gender();
    if(((int)info.raceID != 0) && raceInfo.findByID(info.raceID))
    {
        info.displayName = raceInfo.name;
        info.displayName = info.displayName.replace("_", " ");
        
        const char *genderName = raceInfo.genderName(info.genderID);
        if(genderName)
        {
            info.displayName.append(QString(" (%1)").arg(genderName));
        }
    }
    else
    {
        info.displayName = info.modelName;
    }
}

static bool actorNameLessThan(const ActorInfo &a, const ActorInfo &b)
{
    return a.displayName < b.displayName;
}

void CharacterScene::updateActorList()
{
    m_actors.clear();
    foreach(CharacterPack *pack, m_game->packs()->characterPacks())
    {
        const QMap<QString, CharacterModel *> &actors = pack->models();
        foreach(QString name, actors.keys())
        {
            ActorInfo actorInfo;
            actorInfo.model = actors[name];
            actorInfo.pack = pack;
            actorInfo.modelName = name;
            actorInfo.fill();
            m_actors.append(actorInfo);
        }
    }
    qSort(m_actors.begin(), m_actors.end(), actorNameLessThan);
    emit actorListUpdated();
}

void CharacterScene::clear()
{
    m_game->clear();
    m_player->setModel(NULL);
    updateActorList();
    m_selectedActorIndex = -1;
}

void CharacterScene::update()
{
    GameUpdate gu;
    gu.currentTime = m_game->currentTime();
    gu.sinceLastUpdate = gu.currentTime - m_lastUpdateTime;
    m_game->packs()->update(gu);
    m_player->preMoveUpdate(gu);
    m_player->updateModelMatrix(gu);
    m_player->updateAnimationState(gu);
    m_lastUpdateTime = gu.currentTime;
}

void CharacterScene::draw()
{
    vec4 clearColor(0.6f, 0.6f, 0.9f, 1.0f);
    if(m_renderCtx && m_renderCtx->beginFrame(clearColor))
    {
        drawFrame();
        m_renderCtx->endFrame();
        m_game->setFlag(eGameFrameAction1, false);
        m_game->setFlag(eGameFrameAction2, false);
        m_game->setFlag(eGameFrameAction3, false);
    }
}

RenderProgram * CharacterScene::program(RenderMode renderMode)
{
    ShaderID shader;
    switch(renderMode)
    {
    default:
    case Basic:
        shader = eShaderBasic;
        break;
    case Skinning:
          switch(m_skinningMode)
          {
          default:
          case SoftwareSkinning:
              shader = eShaderBasic;
              break;
          case HardwareSkinning:
              shader = eShaderSkinning;
              break;
          }
    }
    return m_renderCtx ? m_renderCtx->programByID(shader) : NULL;
}

void CharacterScene::drawFrame()
{
    Frustum &viewFrustum = m_camera->frustum();
    viewFrustum.setAspect(m_renderCtx->aspectRatio());
    m_renderCtx->matrix(RenderContext::Projection) = viewFrustum.projection();
    
    vec3 rot = m_theta;
    m_renderCtx->translate(m_delta.x, m_delta.y, m_delta.z);
    m_renderCtx->rotateX(rot.x);
    m_renderCtx->rotateY(rot.y);
    m_renderCtx->rotateZ(rot.z);
    m_renderCtx->scale(m_sigma, m_sigma, m_sigma);
    
    RenderProgram *prog = program(Skinning);
    vec4 ambientLight(1.0, 1.0, 1.0, 1.0);
    m_renderCtx->setCurrentProgram(prog);
    prog->setAmbientLight(ambientLight);
    
    if(m_player->model())
    {
        m_player->setAnimationTime(m_game->currentTime());
        m_player->draw(m_renderCtx, prog);
    }
}

void CharacterScene::keyReleaseEvent(QKeyEvent *e)
{
    int key = e->key();
    if(key == Qt::Key_Q)
        m_theta.y += 5.0;
    else if(key == Qt::Key_D)
        m_theta.y -= 5.0;
    else if(key == Qt::Key_2)
        m_theta.x += 5.0;
    else if(key == Qt::Key_8)
        m_theta.x -= 5.0;
    else if(key == Qt::Key_4)
        m_theta.z += 5.0;
    else if(key == Qt::Key_6)
        m_theta.z -= 5.0;
    else if(key == Qt::Key_F10)
    {
        m_game->setFlag(eGameFrameAction1, true);
    }
    else if(key == Qt::Key_F11)
    {
        m_game->setFlag(eGameFrameAction2, true);
    }
    else if(key == Qt::Key_F12)
    {
        m_game->setFlag(eGameFrameAction3, true);
    }
}

void CharacterScene::mouseMoveEvent(QMouseEvent *e)
{
    int x = e->x();
    int y = e->y();

    if(m_transState.active)
    {
        int dx = m_transState.x0 - x;
        int dy = m_transState.y0 - y;
        m_delta.x = (m_transState.last.x - (dx / 100.0));
        m_delta.z = (m_transState.last.y + (dy / 100.0));
    }

    if(m_rotState.active)
    {
        int dx = m_rotState.x0 - x;
        int dy = m_rotState.y0 - y;
        m_theta.x = (m_rotState.last.x + (dy * 2.0));
        m_theta.z = (m_rotState.last.z + (dx * 2.0));
    }
}

void CharacterScene::mousePressEvent(QMouseEvent *e)
{
    int x = e->x();
    int y = e->y();
    if(e->button() & Qt::MidButton)       // middle button pans the scene
    {
        m_transState.active = true;
        m_transState.x0 = x;
        m_transState.y0 = y;
        m_transState.last = m_delta;
    }
    else if(e->button() & Qt::LeftButton)   // left button rotates the scene
    {
        m_rotState.active = true;
        m_rotState.x0 = x;
        m_rotState.y0 = y;
        m_rotState.last = m_theta;
    }
}

void CharacterScene::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() & Qt::MidButton)
        m_transState.active = false;
    else if(e->button() & Qt::LeftButton)
        m_rotState.active = false;
}

void CharacterScene::wheelEvent(QWheelEvent *e)
{
    // mouse wheel up zooms towards the scene
    // mouse wheel down zooms away from scene
    m_sigma *= pow(1.01, e->delta() / 8);
}

////////////////////////////////////////////////////////////////////////////////

ActorInfo::ActorInfo()
{
    raceID = eRaceInvalid;
    genderID = eGenderMale;
    model = NULL;
    pack = NULL;
}

void ActorInfo::fill()
{
    RaceInfo raceInfo;
    raceID = model->race();
    genderID = model->gender();
    if((raceID != eRaceInvalid) && raceInfo.findByID(raceID))
    {
        displayName = QString(raceInfo.name).replace("_", " ");
        
        const char *genderName = raceInfo.genderName(genderID);
        if(genderName)
        {
            displayName.append(QString(" (%1)").arg(genderName));
        }
        displayName.append(QString(" - %1").arg(modelName));
    }
    else
    {
        displayName = modelName;
    }
}
