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

#include <QScopedPointer>
#include "EQuilibre/Game/ZoneActors.h"
#include "EQuilibre/Game/CharacterActor.h"
#include "EQuilibre/Game/Game.h"
//#include "EQuilibre/Game/GameClient.h"
#include "EQuilibre/Game/GamePacks.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/ZoneTerrain.h"
#include "EQuilibre/Core/Character.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderProgram.h"

ZoneActors::ZoneActors(Zone *zone)
{
    m_zone = zone;
    m_game = zone->game();
    m_movementAheadTime = 0.0f;
    m_player = new CharacterActor(zone, true);
    m_actorTree = NULL;
    m_actorsStat = NULL;
    m_drawnActorsStat = NULL;
    
    const int modelActorSize = 256;
    const int batchActorSize = 32;
    m_modelActors.reserve(modelActorSize);
    m_batchActors.reserve(batchActorSize);
    m_batchMVMatrices.reserve(batchActorSize);
    m_batchMaterialMaps.reserve(batchActorSize);
    m_batchAnimIDs.reserve(batchActorSize);
    m_batchAnimFrames.reserve(batchActorSize);
}

ZoneActors::~ZoneActors()
{
    unloadActors();
    unload();
    delete m_player;
}

void ZoneActors::unloadActors()
{
    m_zone->playerExit();
}

void ZoneActors::unload()
{
    foreach(CharacterActor *actor, m_actors.values())
    {
        actor->setModel(NULL);
        actor->setPack(NULL);
    }
    foreach(CharacterPack *pack, m_charPacks)
        delete pack;
    m_charPacks.clear();
    delete m_actorTree;
    m_actorTree = NULL;
}

CharacterActor * ZoneActors::player() const
{
    return m_player;
}

OctreeIndex * ZoneActors::index() const
{
    return m_actorTree;
}

QList<CharacterPack *> ZoneActors::characterPacks() const
{
    return m_charPacks;
}

const QMap<uint32_t, CharacterActor *> & ZoneActors::actors() const
{
    return m_actors;
}

std::vector<CharacterActor *> & ZoneActors::visibleActors()
{
    return m_visibleActors;
}

Zone * ZoneActors::zone() const
{
    return m_zone;
}

CharacterPack * ZoneActors::loadCharacters(QString archivePath, QString wldName)
{
    CharacterPack *charPack = m_game->packs()->loadCharacters(archivePath, wldName, false);
    if(charPack)
        m_charPacks.append(charPack);
    return charPack;
}

OctreeIndex * ZoneActors::createIndex(const AABox &bounds)
{
    if(!m_actorTree)
        m_actorTree = new OctreeIndex(bounds, 8);
    return m_actorTree;
}

void ZoneActors::spawnPlayer(const SpawnInfo &playerInfo)
{
    m_player->setInfo(playerInfo);
    m_actors[playerInfo.spawnID] = m_player;
}

CharacterActor * ZoneActors::createActor(const SpawnInfo &info)
{
    CharacterActor *actor = new CharacterActor(m_zone);
    actor->setInfo(info);
    m_actors[info.spawnID] = actor;
//    if(m_player->isInZone())
//    {
//        actor->enteredZone();
//    }
    /*qDebug("Created actor '%s' (ID=%d, race=%d, skin=%d, helm=%d, gender=%d, scale=%f)",
           info.name.toLatin1().constData(), info.spawnID, info.raceID,
           info.equip[eSlotChest], info.equip[eSlotHead], info.gender, info.scale);*/
    return actor;
}

CharacterActor * ZoneActors::findActor(uint32_t spawnID)
{
    return m_actors.value(spawnID, NULL);
}

bool ZoneActors::removeActor(uint32_t spawnID)
{
    QMap<uint32_t, CharacterActor *>::iterator I = m_actors.find(spawnID);
    if(I != m_actors.end())
    {
        CharacterActor *actor = I.value();
        m_actors.erase(I);
        if(actor != m_player)
        {
            delete actor;
        }
        return true;
    }
    return false;
}

void ZoneActors::playerEntry()
{
    foreach(CharacterActor *actor, m_actors.values())
    {
        //actor->enteredZone();
    }
}

void ZoneActors::playerExit()
{
    foreach(CharacterActor *actor, m_actors.values())
    {
        uint16_t spawnID = actor->spawnID();
        //actor->leftZone();
        removeActor(spawnID);
    }
    m_actors.clear();
}

void ZoneActors::update(const GameUpdate &gu)
{
    foreach(CharacterActor *actor, m_actors)
        actor->preMoveUpdate(gu);
    
    // Calculate the next character positions using fixed-duration ticks.
    const double tick = (1.0 / Game::MOVEMENT_TICKS_PER_SEC);
    m_movementAheadTime += gu.sinceLastUpdate;
    while(m_movementAheadTime > tick)
    {
        foreach(CharacterActor *actor, m_actors)
        {
            actor->updatePosition(tick);
        }
        m_movementAheadTime -= tick;
    }
    
    // Interpolate the position since we calculated it too far in the future.
    double alpha = (m_movementAheadTime / tick);
    foreach(CharacterActor *actor, m_actors)
    {
        actor->interpolateState(alpha);
        actor->postMoveUpdate(gu);
    }
}

void ZoneActors::updateVisible(const GameUpdate &gu)
{
    // Update the skeleton bone positions for visible characters.
    foreach(CharacterActor *actor, m_visibleActors)
    {
        actor->updateAnimationState(gu);
    }
}

static bool modelLessThan(const CharacterActor *a, const CharacterActor *b)
{
    return a->model() < b->model();
}

void ZoneActors::draw(RenderProgram *prog)
{
    RenderContext *renderCtx = m_game->renderContext();
    if(!renderCtx->isValid())
        return;
    
    // The player is only drawn if far enough away from the camera.
    float cameraDistance = m_zone->camera()->cameraDistance();
    bool drawPlayer = (cameraDistance >= m_game->minDistanceToShowCharacter());
    
    // Render the characters, one batch per model.
    unsigned i = 0;
    qSort(m_visibleActors.begin(), m_visibleActors.end(), modelLessThan);
    do
    {
        // Get all characters with the same model in a batch.
        CharacterModel *model = m_visibleActors[i]->model();
        for(; i < m_visibleActors.size(); i++)
        {
            CharacterActor *actor = m_visibleActors[i];
            if(actor->model() != model)
            {
                break;
            }
            if((actor == m_player) && !drawPlayer)
            {
                continue;
            }
            m_modelActors.push_back(actor);    
        }
        
        // Draw this batch if the model has been uploaded.
        if(model->upload(renderCtx) && (m_modelActors.size() > 0))
        {
            drawModel(prog, model);
        }
        m_modelActors.clear();
    } while(i < m_visibleActors.size());
    
    // Render the player capsule if enabled.
    MeshData *capsule = m_game->packs()->capsule();
    if(capsule && drawPlayer && m_game->hasFlag(eGameDrawCapsule))
    {
        vec3 loc = m_player->location();
        const float modelHeight = 4.0;
        const float modelRadius = 1.0;
        float scaleXY = (m_player->capsuleRadius() / modelRadius);
        float scaleZ = (m_player->capsuleHeight() / modelHeight);
        renderCtx->pushMatrix();
        renderCtx->translate(loc.x, loc.y, loc.z);
        renderCtx->rotateZ(m_player->heading());
        renderCtx->scale(scaleXY, scaleXY, scaleZ);
        m_game->drawBuiltinObject(capsule, prog);
        renderCtx->popMatrix();
    }
}

void ZoneActors::drawModel(RenderProgram *prog, CharacterModel *model)
{
    uint32_t drawnMask = 0;
    uint32_t meshCount = model->meshes().size();
    
    // Determine which meshes are used by the actors.
    // Different skins can have different meshes (e.g. alternate head mesh).
    CharacterActor *actor = m_modelActors[0];
    uint32_t firstMask = model->getSkinMask(actor->skinID());
    uint32_t combinedMaskAND = firstMask;
    uint32_t combinedMaskOR = firstMask;
    for(unsigned i = 1; i < m_modelActors.size(); i++)
    {
        uint32_t mask = model->getSkinMask(actor->skinID());
        actor = m_modelActors[i];
        combinedMaskAND &= mask;
        combinedMaskOR |= mask;
    }
    
    // Draw the meshes that are used by all actors first.
    drawModel(prog, model, combinedMaskAND, drawnMask);
    
    // Draw the remaining meshes, if any.
    unsigned i = 0;
    while((i < meshCount) && (drawnMask != combinedMaskOR))
    {
        // Draw this one mesh for all actors that use it.
        uint32_t drawMask = 0;
        model->setPartUsed(drawMask, i, true);
        drawModel(prog, model, drawMask, drawnMask);
        i++;
    }
}

void ZoneActors::drawModel(RenderProgram *prog, CharacterModel *model,
                           uint32_t drawMask, uint32_t &drawnMask)
{
    // Make sure we do not draw meshes twice.
    drawMask &= ~drawnMask;
    if(!drawMask)
    {
        return;
    }
    
    // Determine which actors to draw.
    for(uint32_t i = 0; i < m_modelActors.size(); i++)
    {
        CharacterActor *actor = m_modelActors[i];
        uint32_t skinMask = model->getSkinMask(actor->skinID());
        if(skinMask & drawMask)
        {
            m_batchActors.push_back(actor);
        }
    }
    if(m_batchActors.size() == 0)
    {
        return;
    }
    
    // Gather material groups from all the mesh parts we want to draw.
    MeshBuffer *meshBuf = model->buffer();
    const std::vector<WLDMesh *> &meshes = model->meshes();
    meshBuf->matGroups.clear();
    for(uint32_t i = 0; i < meshes.size(); i++)
    {
        WLDMesh *mesh = meshes[i];
        if(model->isPartUsed(drawMask, i))
            meshBuf->addMaterialGroups(mesh->data());
    }
    
    // Setup batch state.
    MaterialArray *materials = model->mainMesh()->palette()->array();
    RenderBatch batch(meshBuf, materials);
    batch.setMaterialTexture(materials->arrayTexture());
    batch.setMaterials(materials);
    batch.setAnimations(model->animations());
    batch.setAnimationTexutre(model->animationTexture());
    
    // Compute instance state.
    RenderContext *renderCtx = m_game->renderContext();
    const matrix4 &camera = renderCtx->matrix(RenderContext::ModelView);
    for(uint32_t i = 0; i < m_batchActors.size(); i++)
    {
        CharacterActor *actor = m_batchActors[i];
        matrix4 mvMatrix = camera * actor->modelMatrix();
        uint32_t animID = 0;
        float animFrame = 0.0f;
        Animation *anim = actor->animation();
        if(anim)
        {
            animID = actor->animationID();
            animFrame = (float)anim->frameAtTime(actor->animationTime());
        }
        m_batchMVMatrices.push_back(mvMatrix);
        m_batchMaterialMaps.push_back(actor->materialMap());
        m_batchAnimIDs.push_back(animID);
        m_batchAnimFrames.push_back(animFrame);
    }
    batch.setModelViewMatrices(&m_batchMVMatrices[0]);
    batch.setMaterialMaps(&m_batchMaterialMaps[0]);
    batch.setAnimationIDs(&m_batchAnimIDs[0]);
    batch.setAnimationFrames(&m_batchAnimFrames[0]);
    
    // Draw, at last!
    prog->beginBatch(batch);
    prog->drawInstances(batch, m_batchActors.size());
    prog->endBatch(batch);
    drawnMask |= drawMask;
    
    // Clean up.
    m_batchActors.clear();
    m_batchMVMatrices.clear();
    m_batchMaterialMaps.clear();
    m_batchAnimIDs.clear();
    m_batchAnimFrames.clear();
}

void ZoneActors::resetVisible()
{
    m_visibleActors.clear();
}
