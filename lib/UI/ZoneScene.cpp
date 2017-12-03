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

#include <math.h>
#include <QKeyEvent>
#include <QFont>
#include <QInputDialog>
#include <QMouseEvent>
#include <QWheelEvent>
#include "EQuilibre/UI/ZoneScene.h"
#include "EQuilibre/Core/Character.h"
#include "EQuilibre/Core/Log.h"
#include "EQuilibre/Core/SoundTrigger.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Game/CharacterActor.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/ZoneActors.h"
#include "EQuilibre/Game/ZoneObjects.h"
#include "EQuilibre/Game/ZoneTerrain.h"

ZoneScene::ZoneScene(Game *game) : Scene(game->renderContext())
{
    m_game = game;
    m_zone = game->zone();
    m_player = m_zone->player();
    m_locFont = new QFont();
    m_locFont->setPointSizeF(16.0);
    m_locFont->setWeight(QFont::Bold);
    m_logFont = new QFont();
    m_logFont->setPointSizeF(12.0);
    
    m_staticProgram = NULL;
    m_actorProgram = NULL;
    m_lightingMode = (int)RenderProgram::NoLighting;
    m_rotState.last = vec3();
    m_rotState.active = false;
    
    connect(m_zone, SIGNAL(loading()), this, SLOT(handleZoneLoading()));
    connect(m_zone, SIGNAL(loaded()), this, SLOT(handleZoneLoaded()));
    connect(m_zone, SIGNAL(unloaded()), this, SLOT(handleZoneUnloaded()));
    connect(m_zone, SIGNAL(playerEntered()), this, SLOT(handlePlayerEntered()));
    connect(m_zone, SIGNAL(playerLeft()), this, SLOT(handlePlayerLeft()));
}

ZoneScene::~ZoneScene()
{
    delete m_logFont;
    delete m_locFont;
}

Game * ZoneScene::game() const
{
    return m_game;
}

int ZoneScene::lightingMode() const
{
    return m_lightingMode;
}

void ZoneScene::setLightingMode(int newMode)
{
    m_lightingMode = newMode;
    m_game->setFlag(eGameLighting, newMode != RenderProgram::NoLighting);
}

void ZoneScene::showZone(bool show)
{
    m_game->setFlag(eGameShowTerrain, show);
}

void ZoneScene::showZoneObjects(bool show)
{
    m_game->setFlag(eGameShowObjects, show);
}

void ZoneScene::showZoneActors(bool show)
{
    m_game->setFlag(eGameShowCharacters, show);
}

void ZoneScene::showSky(bool show)
{
    m_game->setFlag(eGameShowSky, show);
}

void ZoneScene::showFog(bool show)
{
    m_game->setFlag(eGameShowFog, show);
}

void ZoneScene::setFrustumCulling(bool enabled)
{
    m_game->setFlag(eGameCullObjects, enabled);
}

void ZoneScene::showSoundTriggers(bool show)
{
    m_game->setFlag(eGameShowSoundTriggers, show);
}

void ZoneScene::enableGPUSkinning(bool enabled)
{
    m_game->setFlag(eGameGPUSkinning, enabled);
}

void ZoneScene::init()
{
    m_zone->camera()->frustum().setFarPlane(2000.0);
    m_staticProgram = m_renderCtx->programByID(eShaderBasic);
    m_actorProgram = m_renderCtx->programByID(eShaderSkinning);
}

void ZoneScene::loadZone(QString path, QString name)
{   
    m_zone->load(path, name);
}

void ZoneScene::handleZoneLoading()
{
    emit loading();
}

void ZoneScene::handleZoneLoaded()
{
    if(!m_game->client())
    {
        // Set default player information in offline mode.
        SpawnInfo playerInfo;
        playerInfo.state.position = m_zone->info().safePos;
        playerInfo.name = "Player";
        playerInfo.classID = eClassWarrior;
        playerInfo.raceID = eRaceHuman;
        playerInfo.gender = eGenderMale;
        playerInfo.type = eSpawnTypePC;
        playerInfo.level = 1;
        playerInfo.runSpeed = 0.7f;
        playerInfo.walkSpeed = playerInfo.runSpeed * 0.5f;
        playerInfo.equip[eSlotPrimary] = 1;
        m_zone->actors()->spawnPlayer(playerInfo);
        m_zone->playerEntry();
    }
}

void ZoneScene::handleZoneUnloaded()
{
}

void ZoneScene::handlePlayerEntered()
{
    emit loaded();
}

void ZoneScene::handlePlayerLeft()
{
    emit unloaded();
}



void ZoneScene::clearText()
{
    m_textElements.clear();
    m_locText.clear();   
    m_tags.clear();
}

void ZoneScene::addLocLine(QString text)
{
    if(!m_locText.isEmpty())
        m_locText.append("\n");
    m_locText.append(text);
}

void ZoneScene::updateTextLayout()
{
    updateLocLayout();
    updateLogLayout();
    foreach(const CharacterTag &tag, m_tags)
    {
        updateTagLayout(tag);
    }
}

void ZoneScene::updateLocLayout()
{
    // Do the layout for the location text area in the top-right.
    QFontMetrics fm(*m_locFont);
    QSizeF locSize(fm.size(0, m_locText));
    QPointF locPos(m_renderCtx->width() - locSize.width() - 10, 5);
    if(!m_locText.isEmpty())
    {
        TextElement element;
        element.text = m_locText;
        element.color = Qt::white;
        element.rect = QRectF(locPos, locSize);
        element.font = *m_locFont;
        m_textElements.append(element);
    }
}

void ZoneScene::updateLogLayout()
{
    // Gather the last lines of the log that fit on the screen.
    if(!m_game->log())
    {
        return;
    }
    QFontMetrics fm(*m_logFont);
    const QVector<QString> &lines = m_game->log()->lines();
    float maxHeight = (m_renderCtx->height() / 2.0f) - 20.0f;
    const float spacing = 4.0f;
    float totalHeight = 0.0f;
    int firstLine = lines.count();
    for(int i = (lines.count() - 1); i >= 0; i--)
    {
        QString line(lines[i]);
        QSizeF lineSize(fm.size(0, line));
        totalHeight += lineSize.height();
        if(totalHeight > maxHeight)
        {
            break;
        }
        totalHeight += spacing;
        firstLine = i;
    }
    if(firstLine >= lines.count())
    {
        return;
    }
    
    // Concatenate the log lines together.
    QString logText;
    for(int i = firstLine; i < lines.count(); i++)
    {
        logText.append(lines[i]);
        logText.append("\n");
    }
    
    // Do the layout for the log text area in the bottom-left.
    QSizeF logSize(fm.size(0, logText));
    QPointF logPos(10.0f, 10.0f + (m_renderCtx->height() / 2));
    if(!logText.isEmpty())
    {
        TextElement element;
        element.text = logText;
        element.color = Qt::white;
        element.rect = QRectF(logPos, logSize);
        element.font = *m_logFont;
        m_textElements.append(element);
    }
}

void ZoneScene::updateTagLayout(const CharacterTag &tag)
{
    QFont font;
    font.setPointSizeF(tag.fontSize);
    font.setWeight(QFont::Bold);
    
    QFontMetrics fm(font);
    QSizeF tagSize(fm.size(0, tag.text));
    QPointF tagOrigin(tag.pos.x() - (tagSize.width() * 0.5f),
                      tag.pos.y() - (tagSize.height() * 0.5f));

    /*TextElement tagShadowEl;
    tagShadowEl.text = tag.text;
    tagShadowEl.font = font;
    tagShadowEl.color = Qt::cyan;
    tagShadowEl.rect = QRectF(tagOrigin + QPointF(1.0f, 1.0f), tagSize);
    m_textElements.append(tagShadowEl);*/
    
    TextElement tagEl;
    tagEl.text = tag.text;
    tagEl.font = font;
    tagEl.color = Qt::blue;
    tagEl.rect = QRectF(tagOrigin, tagSize);
    m_textElements.append(tagEl);
}

void ZoneScene::update()
{
    clearText();
    m_game->update();
    
    Frustum &viewFrustum = m_zone->camera()->frustum();
    matrix4 proj = viewFrustum.projection();
    m_renderCtx->matrix(RenderContext::Projection) = proj;
    if(!m_zone->isLoaded())
        return;
    
    if(m_game->client())
    {
        addLocLine(QString("%1 (HP: %2/%3)")
            .arg(m_player->name())
            .arg(m_player->currentHP())
            .arg(m_player->maxHP()));
    }
    
    vec3 playerPos = m_player->location();
    float playerHeading = m_player->heading();
    addLocLine(QString("%1 %2 %3 @ %4")
        .arg(playerPos.x, 0, 'f', 2)
        .arg(playerPos.y, 0, 'f', 2)
        .arg(playerPos.z, 0, 'f', 2)
        .arg(playerHeading, 0, 'f', 2));
    
    uint32_t currentRegion = m_zone->terrain()->currentRegionID();
    if(currentRegion)
        addLocLine(QString("Current region: %1").arg(currentRegion));
    
    if(m_game->hasFlag(eGameShowSoundTriggers))
    {
        QVector<SoundTrigger *> triggers;
        m_zone->currentSoundTriggers(triggers);
        foreach(SoundTrigger *trigger, triggers)
        {
            SoundEntry e = trigger->entry();
            addLocLine(QString("Sound Trigger %1 (%2-%3)").arg(e.ID).arg(e.SoundID1).arg(e.SoundID2));
        }
    }
    
    matrix4 devMatrix = matrix4::scale(m_renderCtx->width() * 0.5f,
                                       m_renderCtx->height() * 0.5f,
                                       1.0f);
    devMatrix = devMatrix * matrix4::translate(1.0f, 1.0f, 1.0f);
    
    const std::vector<CharacterActor *> &actors = m_zone->actors()->visibleActors();
    for(unsigned i = 0; i < actors.size(); i++)
    {
        // Determine the tag font size based on the distance from the camera.
        CharacterActor *actor = actors[i];
        vec3 tagPos = actor->tagPos();
        float tagDistSquared = (tagPos - viewFrustum.eye()).lengthSquared();
        float size = 100.0f * powf(tagDistSquared, -0.25);
        const float minSize = 8.0f;
        if(size < minSize)
        {
            continue;
        }
        
        // Find the screen position of the player's tag.
        vec3 tagPosEye = viewFrustum.camera().map(tagPos);
        vec3 tagClip = proj.map(tagPosEye);
        float x = ((tagClip.x / tagClip.z) + 1.0f) * m_renderCtx->width() * 0.5f;
        float y = ((tagClip.y / -tagClip.z) + 1.0f) * m_renderCtx->height() * 0.5f;
        
        // Do not display the character's tag when it is behind the camera.
        if(tagClip.z > 1.0f)
        {
            continue;
        }

        CharacterTag tag;
        tag.text = actor->name();
        tag.pos = QPointF(x, y);
        tag.fontSize = size;
        m_tags.append(tag);
    }
    
    updateTextLayout();
}

void ZoneScene::draw()
{
    vec4 clearColor = m_zone->isLoaded() ? m_zone->info().fogColor : vec4(0.4f, 0.4f, 0.6f, 0.1f);
    if(m_renderCtx->beginFrame(clearColor))
    {
        drawFrame();
        m_renderCtx->endFrame();
        m_game->setFlag(eGameFrameAction1, false);
        m_game->setFlag(eGameFrameAction2, false);
        m_game->setFlag(eGameFrameAction3, false);
    }
}

void ZoneScene::drawFrame()
{
    if(!m_zone->isLoaded())
        return;
    
    // Upload zone ressources.
    ZoneTerrain *terrain = m_zone->terrain();
    ZoneObjects *objects = m_zone->objects();
    ZoneSky *sky = m_game->sky();
    terrain->upload();
    objects->upload();
    sky->upload();
    
    // Make sure we have uploaded everything we need.
    bool terrainReady = (terrain->state() == eAssetUploaded) || !m_game->hasFlag(eGameShowTerrain);
    bool objectsReady = (objects->state() == eAssetUploaded) || !m_game->hasFlag(eGameShowObjects);
    bool skyReady = (sky->state() == eAssetUploaded) || !m_game->hasFlag(eGameShowSky);
    if(terrainReady && objectsReady && skyReady)
    {
        // Draw the zone.
        m_staticProgram->setLightingMode((RenderProgram::LightingMode)m_lightingMode);
        m_zone->draw(m_staticProgram, m_actorProgram);
    }
    else
    {
        // Show loading text.
        TextElement el;
        el.text = "LOADING, PLEASE WAIT...";
        el.color = Qt::black;
        el.font = *m_locFont;
        
        QFontMetrics fm(el.font);
        QPointF pos(10.0f, 10.0);
        QSizeF size = fm.size(0, el.text);
        el.rect = QRectF(pos, size);
        
        m_textElements.push_back(el);
    }
}

void ZoneScene::keyPressEvent(QKeyEvent *e)
{
    CharacterActor *player = m_zone->player();
    int newMovementStraight = 0;
    int newMovementSideways = 0;
    int key = e->key();
    if(key == Qt::Key_A)
    {
        newMovementSideways = 1;
    }
    else if(key == Qt::Key_D)
    {
        newMovementSideways = -1;
    }
    else if(key == Qt::Key_W)
    {
        newMovementStraight = 1;
    }
    else if(key == Qt::Key_S)
    {
        newMovementStraight = -1;
    }
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
    if(e->modifiers() & Qt::ShiftModifier)
    {

    }
    

}

void ZoneScene::keyReleaseEvent(QKeyEvent *e)
{
    CharacterActor *player = m_zone->player();
    bool releaseStraight = false;
    bool releaseSideways = false;
    int key = e->key();
    switch(key)
    {
    case Qt::Key_W:
    case Qt::Key_S:
        releaseStraight = !e->isAutoRepeat();
        break;
    case Qt::Key_A:
    case Qt::Key_D:
        releaseSideways = !e->isAutoRepeat();
        break;
    case Qt::Key_E:

        break;
    case Qt::Key_Space:
        if(m_game->hasFlag(eGameFrustumIsFrozen))
            m_game->unFreezeFrustum();
        else
            m_game->freezeFrustum();
        break;
    case Qt::Key_G:
        m_game->toggleFlag(eGameApplyGravity);
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        //showTextInputDialog();
        break;
    }

}

void ZoneScene::mouseMoveEvent(QMouseEvent *e)
{
    CharacterActor *player = m_zone->player();

    
    if(m_rotState.active)
    {
        int dx = m_rotState.x0 - e->x();
        int dy = m_rotState.y0 - e->y();
        float newHeading = (m_rotState.last.x + (dx * 1.0));
        float newPitch = (m_rotState.last.y + (dy * 1.0));
        newPitch = qMin(qMax(newPitch, -85.0f), 85.0f);
        m_zone->camera()->setLookPitch(newPitch);
        player->setHeading(newHeading);
    }
}

void ZoneScene::mousePressEvent(QMouseEvent *e)
{
    CharacterActor *player = m_zone->player();

    
    if(e->button() & Qt::RightButton)
    {
        m_rotState.active = true;
        m_rotState.x0 = e->x();
        m_rotState.y0 = e->y();
        m_rotState.last = vec3(player->heading(), m_zone->camera()->lookPitch(), 0.0f);
    }
}

void ZoneScene::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() & Qt::RightButton)
        m_rotState.active = false;
}

void ZoneScene::wheelEvent(QWheelEvent *e)
{
    CharacterActor *player = m_zone->player();

    
    Camera *camera = m_zone->camera();
    float distance = camera->cameraDistance();
    float delta = (e->delta() * 0.01);
    distance = qMax(distance - delta, 0.0f);
    camera->setCameraDistance(distance);
 }

