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
#include "EQuilibre/Game/CharacterActor.h"
#include "EQuilibre/Game/Game.h"
//#include "EQuilibre/Game/GameClient.h"
#include "EQuilibre/Game/GamePacks.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/ZoneActors.h"
#include "EQuilibre/Game/ZoneObjects.h"
#include "EQuilibre/Game/ZoneTerrain.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderProgram.h"

CharacterActor::CharacterActor(Zone *zone, bool player) : Actor(Kind, zone)
{
    m_game = zone->game();
    m_player = player;
    m_pack = NULL;
    m_model = NULL;
    m_materialMap = NULL;
    m_animation = NULL;
    m_currentRegion = NULL;
    m_location = vec3(0.0, 0.0, 0.0);
    m_movementStraight = m_movementSideways = 0;
    m_scale = 1.0f;
    m_inZone = false;
    m_lastUpdateTime = 0.0f;
    m_heading = 0.0f;
    m_spawnAnim = eSpawnAnimStanding;
    m_animationID = eAnimInvalid;
    m_queuedAnimationID = eAnimInvalid;
    m_repeatAnim = false;
    m_walk = false;
    m_wantsToJump = false;
    m_jumping = false;
    m_jumpTime = 0.0f;
    m_animTime = 0.0f;
    m_startAnimationTime = 0.0f;
    m_capsuleHeight = 6.0;
    m_capsuleRadius = 1.0;
    m_currentHP = 0;
    m_maxHP = 0;
    for(unsigned i = 0; i < eTrackCount; i++)
    {
        m_trackObject[i] = NULL;
        m_trackID[i] = -1;
    }
}

CharacterActor::~CharacterActor()
{
    setCurrentRegion(NULL);
    delete m_materialMap;
}

Game * CharacterActor::game() const
{
    return m_game;
}


bool CharacterActor::isPlayer() const
{
    return m_player;
}

bool CharacterActor::isCorpse() const
{
    return ((m_info.type == eSpawnTypeNPCCorpse) ||
            (m_info.type == eSpawnTypePCCorpse));
}

RegionActor * CharacterActor::currentRegion() const
{
    return m_currentRegion;
}

void CharacterActor::setCurrentRegion(RegionActor *newRegion)
{
    if(m_currentRegion)
        m_currentRegion->removeCharacter(this);
    m_currentRegion = newRegion;
    if(newRegion)
        newRegion->addCharacter(this);
}

CharacterPack * CharacterActor::pack() const
{
    return m_pack;
}

void CharacterActor::setPack(CharacterPack *newPack)
{
    m_pack = newPack;
}

CharacterModel * CharacterActor::model() const
{
    return m_model;
}

void CharacterActor::setModel(CharacterModel *newModel)
{
    if(newModel != m_model)
    {
        if(newModel && !m_materialMap)
        {
            m_materialMap = new MaterialMap();
        }
        if(newModel)
        {
            WLDMaterialPalette *pal = newModel->mainMesh()->palette();
            m_materialMap->clear();
            m_materialMap->resize(pal->materialSlots().size());
            m_model = newModel;
        }
        else
        {
            m_model = NULL;
        }
    }
}

float CharacterActor::capsuleHeight() const
{
    return m_capsuleHeight;
}

float CharacterActor::capsuleRadius() const
{
    return m_capsuleRadius;
}

uint32_t CharacterActor::spawnID() const
{
    return m_info.spawnID;
}

QString CharacterActor::name() const
{
    return m_name;
}

const SpawnInfo & CharacterActor::info() const
{
    return m_info;
}

static BoneTrackID trackIDFromSlotID(SlotID slotID)
{
    switch(slotID)
    {
    default:
        return eTrackCount;
    case eSlotPrimary:
        return eTrackRight;
    case eSlotSeconday:
        return eTrackLeft;
    }
}

void CharacterActor::setInfo(const SpawnInfo &newInfo)
{
    QRegExp numExpr("\\d+");
    m_info = newInfo;
    m_name = newInfo.name;
    m_name = m_name.replace("_", " ").replace(numExpr, "");
    for(unsigned i = 0; i < eSlotCount; i++)
    {
        setEquip((SlotID)i, newInfo.equip[i], newInfo.equipColors[i]);
    }
}

void CharacterActor::setEquip(SlotID slotID, uint32_t value, uint32_t color)
{
    // Delete any previous equipment object.
    ZoneObjects *objects = m_zone->objects();
    BoneTrackID trackID = trackIDFromSlotID(slotID);
    uint32_t oldEquip = m_info.equip[slotID];
    if(oldEquip && (trackID < eTrackCount))
    {
        ObjectActor *object = m_trackObject[trackID];
        if(object)
        {
            delete object;
            m_trackObject[trackID] = NULL;
        }
    }
    
    // Create a new equipment object if needed.
    if(value && (trackID < eTrackCount))
    {
        m_trackObject[trackID] = objects->createEquip(value);
    }
    
    m_info.equip[slotID] = value;
    m_info.equipColors[slotID] = color;
}

int CharacterActor::currentHP() const
{
    return m_currentHP;
}

int CharacterActor::maxHP() const
{
    return m_maxHP;
}

bool CharacterActor::isWalking() const
{
    return m_walk;
}

void CharacterActor::setWalking(bool isWalking)
{
    m_walk = isWalking;
    updateAnimSpeed();
}

float CharacterActor::speed() const
{
    float multipliyer = m_player ? 2.0f : 1.0f;
    float baseSpeed;
    switch(m_spawnAnim)
    {
    default:
    case eSpawnAnimStanding:
        baseSpeed = (m_walk ? m_info.walkSpeed : m_info.runSpeed);
        break;
    case eSpawnAnimDucking:
        baseSpeed = m_info.walkSpeed;
        break;
    case eSpawnAnimLooting:
    case eSpawnAnimSitting:
    case eSpawnAnimFeigningDeath:
        baseSpeed = 0.0f;
        break;
    }
    baseSpeed *= multipliyer;
    return baseSpeed;
}

float CharacterActor::speedFactor() const
{
    float base = Game::NET_VELOCITY_RATIO;
    if(m_info.type == eSpawnTypePC)
    {
        base *= Game::PLAYER_VELOCITY_MOD;
    }
    return base;
}

bool CharacterActor::jumping() const
{
    return m_jumping;
}

void CharacterActor::setJumping(bool isJumping)
{
    m_jumping = isJumping;
}

const SpawnState & CharacterActor::currentState() const
{
    return m_info.state;
}

SpawnState & CharacterActor::currentState()
{
    return m_info.state;
}

SpawnState & CharacterActor::previousState()
{
    return m_previousState;
}

int CharacterActor::movementStraight() const
{
    return m_movementStraight;
}

int CharacterActor::movementSideways() const
{
    return m_movementSideways;
}


void CharacterActor::updateAnimSpeed()
{
    int animSpeed = (int)(speed() * speedFactor());
    SpawnState &cur = currentState();
    cur.speed = (m_movementStraight || m_movementStraight) ? animSpeed : 0;
}

float CharacterActor::heading() const
{
    return m_heading;
}

void CharacterActor::setHeading(float newHeading)
{
    m_heading = currentState().heading = newHeading;
}

vec3 CharacterActor::tagPos() const
{
    const vec3 tagOffset(0.0f, 0.0f, 1.0f);
    return m_trackPos[eTrackHead] + tagOffset;
}

const matrix4 & CharacterActor::modelMatrix() const
{
    return m_modelMatrix;
}

MaterialMap * CharacterActor::materialMap() const
{
    return m_materialMap;
}

Animation * CharacterActor::animation() const
{
    return m_animation;   
}

void CharacterActor::setAnimation(Animation *newAnim, CharacterAnimation newAnimID)
{
    if(newAnim != m_animation)
    {
        m_animation = newAnim;
        for(unsigned i = 0; i < eTrackCount; i++)
        {
            if(m_animation)
                m_trackID[i] = m_animation->findTrack(trackName((BoneTrackID)i));
            else
                m_trackID[i] = -1;
        }
    }
    m_animationID = newAnimID;
}

uint32_t CharacterActor::animationID()
{
    return m_animationID;
}

double CharacterActor::animationTime() const
{
    return m_animTime;
}

void CharacterActor::setAnimationTime(double newTime)
{
    m_animTime = newTime;
}

uint32_t CharacterActor::skinID() const
{
    return m_info.equip[eSlotHead];
}

void CharacterActor::setSkinID(uint32_t newID)
{
    m_info.equip[eSlotHead] = newID;
}

QString CharacterActor::trackName(BoneTrackID trackID)
{
    switch(trackID)
    {
    case eTrackHead:
        return "HEAD_POINT";
    case eTrackGuild:
        return "GUILD_POINT";
    case eTrackLeft:
        return "L_POINT";
    case eTrackRight:
        return "R_POINT";
    case eTrackShield:
        return "SHIELD_POINT";
    }
    return QString::null;
}

Animation * CharacterActor::findAnimation(QString animName)
{
    if(!m_model || !m_model->skeleton())
    {
        return NULL;
    }
    return m_model->skeleton()->animations().value(animName);
}

uint32_t CharacterActor::skinIDForSlot(WLDMaterialSlot *slot, uint32_t baseID) const
{
    // With NPCs, only head and chest textures affect the model.
    SlotID slotID = slot->slotID;
    if((m_info.type != eSpawnTypePC) && (m_info.type != eSpawnTypePCCorpse))
    {
        slotID = (slotID != eSlotHead) ? eSlotChest : eSlotHead;
    }
    
    // Return the slot's skin ID, if it was identified.
    return (slotID < eSlotCount) ? m_info.equip[slotID] : baseID;
}

void CharacterActor::updateSkinTextures(WLDMaterialPalette *pal, uint32_t baseID)
{
    std::vector<WLDMaterialSlot *> &matSlots = pal->materialSlots();
    size_t endSkinID = qMin(matSlots.size(), m_materialMap->count());
    for(size_t i = 0; i < endSkinID; i++)
    {
        WLDMaterialSlot *slot = matSlots[i];
        uint32_t slotSkinID = skinIDForSlot(slot, baseID);
        uint32_t mappedID = pal->findMaterialIndex(slot, slotSkinID);
        m_materialMap->setMappingAt(i, mappedID);
    }
}

void CharacterActor::draw(RenderContext *renderCtx, RenderProgram *prog)
{
    if(!m_model || !m_model->upload(renderCtx))
    {
        return;
    }
    uint32_t baseID = skinID();
    uint32_t animID = 0;
    float animFrame = 0.0f;
    if(m_animation)
    {
        animID = m_animationID;
        animFrame = (float)m_animation->frameAtTime(m_animTime);
    }
    
    // Gather material groups from all the mesh parts we want to draw.
    MeshBuffer *meshBuf = m_model->buffer();
    const std::vector<WLDMesh *> &meshes = m_model->meshes();
    uint32_t skinMask = m_model->getSkinMask(baseID);
    meshBuf->matGroups.clear();
    for(uint32_t i = 0; i < meshes.size(); i++)
    {
        WLDMesh *mesh = meshes[i];
        if(m_model->isPartUsed(skinMask, i))
            meshBuf->addMaterialGroups(mesh->data());
    }
    
    // Draw all the material groups in one draw call.
    const matrix4 &camera = renderCtx->matrix(RenderContext::ModelView);
    matrix4 mvMatrix = camera * m_modelMatrix;
    MaterialArray *materials = m_model->mainMesh()->palette()->array();
    RenderBatch batch(meshBuf, materials);
    batch.setModelViewMatrices(&mvMatrix);
    batch.setMaterialTexture(materials->arrayTexture());
    batch.setMaterialMaps(&m_materialMap);
    batch.setAnimations(m_model->animations());
    batch.setAnimationTexutre(m_model->animationTexture());
    batch.setAnimationIDs(&animID);
    batch.setAnimationFrames(&animFrame);
    prog->drawMesh(batch);
}

void CharacterActor::jump()
{
    if(!m_jumping || m_game->hasFlag(eGameAllowMultiJumps))
    {
        m_wantsToJump = true;
    }
}

void CharacterActor::interpolateState(double alpha)
{
    SpawnState &cur = currentState();
    SpawnState &prev = previousState();
    m_location = (cur.position * alpha) + (prev.position * (1.0 - alpha));
    m_heading = (cur.heading * alpha) + (prev.heading * (1.0 - alpha));
}

void CharacterActor::updatePosition(double dt)
{
    SpawnState &cur = currentState();
    previousState() = cur;
    
    bool applyGravity = m_game->hasFlag(eGameApplyGravity);
    if(isPlayer())
    {
        // Handle jumping.
        const float jumpDuration = 0.2f;
        const float jumpAccelFactor = 2.5f;
        if(m_wantsToJump)
        {
            m_jumpTime = jumpDuration;
            m_jumping = true;
            m_wantsToJump = false;
        }
        double jumpTime = qMin((double)m_jumpTime, dt);
        if(m_jumping && (jumpTime > 0.0))
        {
            double jumpAccel = (-jumpAccelFactor * m_game->gravity().z) * dt;
            cur.velocity = cur.velocity + vec3(0.0, 0.0, (float)jumpAccel);
            m_jumpTime -= jumpTime;
        }
        
        // Perform a step if the player is moving.
        float baseVelocity = (speed() * speedFactor());
        float deltaX = (m_movementStraight * baseVelocity);
        float deltaY = (m_movementSideways * baseVelocity);
        matrix4 m = m_zone->camera()->movementMatrix();
        cur.innerVelocity = m.map(vec3(deltaX, deltaY, 0.0));
    }
    else
    {
        applyGravity &= applyGravity = m_game->hasFlag(eGameApplyGravityNet);
    }
    
    // Handle gravity.
    if(applyGravity)
    {
        cur.velocity = cur.velocity + (m_game->gravity() * dt);
    }
    cur.position = cur.position + cur.velocity;
    cur.position = cur.position + (cur.innerVelocity * dt);
    
    if(applyGravity)
    {
        handleCollisions();
    }
}

void CharacterActor::handleCollisions()
{
//    SpawnState &cur = currentState();
//    if(!m_zone || !m_shape || !m_inZone)
//    {
//        // No collision without a zone.
//        return;
//    }
    
//    // Make the capsule upright.
//    matrix4 charTransform = matrix4::translate(cur.position);
//    charTransform = charTransform * matrix4::rotate(90.0, 0.0, 1.0, 0.0);
    
//    // Find regions that are possibly intersecting with the character.
//    // Using a bounding sphere should not increase the number of regions to
//    // check too much and it makes the query faster.
//    const int MAX_NEARBY_REGIONS = 128;
//    uint32_t nearbyRegions[MAX_NEARBY_REGIONS];
//    matrix4 regionTransform = matrix4::translate(0.0, 0.0, 0.0);
//    Sphere playerSphere(cur.position, m_capsuleHeight * 0.5f);
//    ZoneTerrain *terrain = m_zone->terrain();
//    uint32_t regionsFound = terrain->findRegions(playerSphere, nearbyRegions,
//                                                 MAX_NEARBY_REGIONS);
    
//    // Check for collisions between the character and nearby regions.
//    vec3 responseVelocity;
//    const int MAX_CONTACTS = 1;
//    vec3 contacts[MAX_CONTACTS];
//    vec3 normals[MAX_CONTACTS];
//    float penetration[MAX_CONTACTS] = {0};
//    NewtonWorld *world = m_zone->collisionWorld();
//    for(uint32_t i = 0; i < regionsFound; i++)
//    {
//        uint32_t regionID = nearbyRegions[i];
//        RegionActor *region = terrain->regionActor(regionID);
//        if(!region)
//        {
//            //qDebug("warning: no actor for region %d", regionID);
//            continue;
//        }
//        NewtonCollision *regionShape = region->loadShape();
//        if(!regionShape)
//        {
//            qDebug("warning: no collision shape for region %d", regionID);
//            continue;
//        }
//        int hits = NewtonCollisionCollide(world, MAX_CONTACTS,
//            m_shape, (const float *)charTransform.columns(),
//            regionShape, (const float *)regionTransform.columns(),
//            (float *)contacts, (float *)normals, penetration, 0);
//        if(hits > 0)
//        {
//            /*qDebug("Collision with player (%f, %f, %f) at (%f, %f, %f) normal (%f, %f, %f) depth %f",
//                   pos.x, pos.y, pos.z,
//                   contacts[0].x, contacts[0].y, contacts[0].z,
//                   normals[0].x, normals[0].y, normals[0].z,
//                   penetration[0]);*/
//            // Detect collisions with zonepoint regions.
//            bool zonePoint = (region && (region->regionType() == eRegionZonePoint));
//            if(zonePoint && isPlayer() && (m_game->client()))
//            {
//                m_game->client()->changeZone(region->zonePointID());
//            }
//            responseVelocity = responseVelocity - (normals[0] * penetration[0]);
//        }
//        m_zone->newCollisionCheck();
//    }
    
//    // Apply the collision response to the character.
//    if((responseVelocity.z > 1e-4) || !m_game->hasFlag(eGameApplyGravity))
//    {
//        // Clear the player's velocity when the ground is hit.
//        cur.velocity.z = 0.0f;
//        m_jumping = false;
//    }
//    cur.position = cur.position + responseVelocity;
}

void CharacterActor::sendClientUpdate(const GameUpdate &gu)
{
//    GameClient *client = m_game->client();
//    if(!isPlayer() || !client)
//    {
//        return;
//    }
    
//    // Update if too much time elapsed since the last update.
//    SpawnState &prev = m_lastSentState;
//    SpawnState &cur = currentState();
//    const double minUpdateInterval = 0.1f;
//    const double maxUpdateInterval = 1.0f;
//    double elapsed = (gu.currentTime - m_lastUpdateTime);
//    bool needUpdate = (elapsed > maxUpdateInterval);
//    bool canUpdate = (elapsed > minUpdateInterval);
//    bool debugLog = false;
    
//    // Update if the difference between previous and current headings is too large.
//    const float maxHeadingDelta = 30.0f;
//    float headingDelta = (cur.heading - prev.heading);
//    if(fabsf(headingDelta) > maxHeadingDelta)
//    {
//        needUpdate = true;
//        if(canUpdate && debugLog)
//        {
//            qDebug("[%f] Heading delta too large: %f, previous: %f, actual: %f",
//                   gu.currentTime, headingDelta, prev.heading, cur.heading);
//        }
//    }
    
//    // Update if the distance between predicted and current position is too large.
//    const float maxDist = 2.0f;
//    vec3 predictedPos = prev.position + (prev.innerVelocity * elapsed);
//    float posDistSquared = (cur.position - predictedPos).lengthSquared();
//    if(posDistSquared > (maxDist * maxDist))
//    {
//        needUpdate = true;
//        if(canUpdate && debugLog)
//        {
//            qDebug("[%f] Pos delta too large %f, predicted: %f %f %f, actual: %f %f %f",
//                   gu.currentTime, sqrt(posDistSquared),
//                   predictedPos.x, predictedPos.y, predictedPos.z,
//                   cur.position.x, cur.position.y, cur.position.z);
//        }
//    }
    
//    // Cap the update interval so that we do not send updates too often.
//    if(needUpdate && canUpdate)
//    {
//        SpawnState &up = m_lastSentState;
//        up = cur;
//        up.innerVelocity = cur.innerVelocity * (1.0f / speedFactor());
//        if(debugLog)
//        {
//            qDebug("[%f] Sending update (pos: %f %f %f heading: %f vel: %f %f %f speed: %d)",
//                   gu.currentTime, up.position.x, up.position.y, up.position.z, up.heading,
//                   up.innerVelocity.x, up.innerVelocity.y, up.innerVelocity.z, up.speed);
//        }
//        client->sendPlayerUpdate(spawnID(), up);
//        cur.sequence++;
//        m_lastUpdateTime = gu.currentTime;
//    }
}

void CharacterActor::preMoveUpdate(const GameUpdate &gu)
{
//    // Show the right model for the race and gender.
//    if(!m_model && m_inZone)
//    {
//        QString actorName = "HUM";
//        RaceInfo race;
//        if(race.findByID(m_info.raceID))
//            actorName = (m_info.gender == eGenderFemale) ? race.modelFemale : race.modelMale;
//        m_pack = m_game->packs()->findCharacterPack(actorName);
//        if(m_pack)
//            setModel(m_pack->models().value(actorName));
//    }
    
//    // Nothing more to update without a model.
//    if(!m_model)
//    {
//        return;
//    }
//    updateSkinTextures(m_model->mainMesh()->palette(), skinID());
    
//    // Create a collision shape if we know the character's volume.
//    Skeleton *skel = m_model->skeleton();
//    if(!m_shape && skel && m_inZone)
//    {
//        // XXX Fix player scales properly.
//        float arbitraryScale = 0.673f;
//        m_capsuleHeight = (skel->boundingRadius() * 2.0f * arbitraryScale);
//        m_capsuleRadius = (m_capsuleHeight / 6.0);
//        m_shape = NewtonCreateCapsule(m_zone->collisionWorld(),
//                                      m_capsuleRadius, m_capsuleHeight, 0, NULL);
//    }
}

void CharacterActor::postMoveUpdate(const GameUpdate &gu)
{
    RegionActor *newRegion = m_zone->terrain()->findRegionActor(m_location);
    if(newRegion != m_currentRegion)
        setCurrentRegion(newRegion);
    updateModelMatrix(gu);
    updateAnimation(gu);
    sendClientUpdate(gu);
}

void CharacterActor::updateModelMatrix(const GameUpdate &gu)
{
    float arbitraryScale = 1.33f; // XXX Fix actor scales properly.
    m_scale = 1.0f;
    if((m_info.type == eSpawnTypeNPC) && (m_capsuleHeight > 0.0))
        m_scale = (m_info.scale / m_capsuleHeight);
    m_scale *= arbitraryScale;
    m_modelMatrix = matrix4::translate(m_location);
    m_modelMatrix = m_modelMatrix * matrix4::rotate(m_heading, 0.0f, 0.0f, 1.0f);
    m_modelMatrix = m_modelMatrix * matrix4::scale(m_scale, m_scale, m_scale);
}

void CharacterActor::updateAnimation(const GameUpdate &gu)
{
    if(!m_model)
    {
        return;
    }
    
    // Update the character's current animation.
    bool lastFrame = false;
    bool repeatAnim = true;
    CharacterAnimation oldAnimID = m_animationID;
    CharacterAnimation newAnimID = nextAnimation(repeatAnim, repeatAnim);
    if(oldAnimID != newAnimID)
    {
        Animation *newAnimation = m_model->animations()->animation(newAnimID);
        setAnimation(newAnimation, newAnimID);
        m_repeatAnim = repeatAnim;
        m_startAnimationTime = gu.currentTime;
    }
    m_animTime = (gu.currentTime - m_startAnimationTime);
    if(m_animation && m_animation->frameCount())
    {
        uint32_t lastFrameID = (m_animation->frameCount() - 1);
        double endTime = m_animation->timeAtFrame(lastFrameID);
        if(m_queuedAnimationID && (m_animTime > endTime) &&
          (m_animationID == m_queuedAnimationID))
        {
            m_queuedAnimationID = eAnimInvalid;
        }
        else if(lastFrame)
        {
            m_animTime = endTime;
        }
        else if(!m_repeatAnim)
        {
            m_animTime = qMin(m_animTime, endTime);
        }
    }
}

void CharacterActor::updateAnimationState(const GameUpdate &gu)
{
    if(!m_model || !m_animation)
    {
        return;
    }
    
    AnimationArray *animArray = m_model->animations();
    for(unsigned i = 0; i < eTrackCount; i++)
    {
        int trackID = m_trackID[i];
        if(trackID < 0)
        {
            continue;
        }
        
        // Calculate the track's model matrix and current position.
        BoneTransform trans = BoneTransform::identity();
        matrix4 &m = m_trackMatrix[i];
        animArray->transformationAtTime(m_animationID, trackID, m_animTime, trans);
        m = m_modelMatrix;
        m = m * matrix4::translate(trans.location);
        m = m * matrix4::rotate(trans.rotation);
        m_trackPos[i] = m.map(vec3(0.0f, 0.0f, 0.0f));
        
        // Update the position/rotation for any equipped object.
        ObjectActor *object = m_trackObject[i];
        if(object)
        {
            object->setLocation(m_location);
            object->setRotation(vec4::quatFromEuler(vec3(0.0f, 0.0f, m_heading)));
            object->setScale(vec3(m_scale, m_scale, m_scale));
            object->setInnerTranslation(trans.location);
            object->setInnerRotation(trans.rotation);
            object->update();
        }
    }
}

void CharacterActor::markEquipVisible()
{
    // Do not show the player's equipment unless it is shown too.
    float cameraDistance = m_zone->camera()->cameraDistance();
    if(isPlayer() && (cameraDistance <= m_game->minDistanceToShowCharacter()))
    {
        return;
    }
    
    QVector<ObjectActor *> &visible = m_zone->objects()->visibleObjects();
    for(unsigned i = 0; i < eTrackCount; i++)
    {
        ObjectActor *object = m_trackObject[i];
        if(object)
        {
            visible.push_back(object);
        }
    }
}

CharacterAnimation CharacterActor::nextAnimation(bool &repeatAnim, bool &lastFrame) const
{
    lastFrame = false;
    repeatAnim = true;
    if(m_jumping)
    {
        return eAnimJump;
    }
    else if(m_queuedAnimationID != eAnimInvalid)
    {
        repeatAnim = false;
        return m_queuedAnimationID;
    }
    else if(isCorpse())
    {
        lastFrame = true;
        repeatAnim = false;
        return eAnimDeath;
    }
    
    // Normalize animation speeds for PCs and NPCs.
    int animSpeed = currentState().speed;
    if(m_info.type == eSpawnTypeNPC)
    {
        animSpeed /= 2;
    }
    
    switch(m_spawnAnim)
    {
    default:
    case eSpawnAnimStanding:
        if(animSpeed <= 0)
        {
            return eAnimIdle;
        }
        else if((animSpeed > 0) && (animSpeed <= 20))
        {
            return eAnimWalk;
        }
        else if((animSpeed > 20) && (animSpeed <= 40))
        {
            return eAnimRun;
        }
        else
        {
            return eAnimSprint;
        }
    case eSpawnAnimDucking:
        return eAnimCrouch;
    case eSpawnAnimLooting:
        repeatAnim = false;
        return eAnimLoot;
    case eSpawnAnimSitting:
        repeatAnim = false;
        return eAnimSit;
    case eSpawnAnimFeigningDeath:
        repeatAnim = false;
        return eAnimDeath; // XXX
    }
}



void CharacterActor::setState(const SpawnState &newState)
{
    SpawnState &cur = currentState();
    cur = newState;
    cur.innerVelocity = cur.innerVelocity * speedFactor();
}

void CharacterActor::move(const vec3 &newPosition, float newHeading)
{
    SpawnState &cur = currentState();
    SpawnState &prev = previousState();
    cur.position = prev.position = newPosition;
    cur.heading = prev.heading = newHeading;
}

void CharacterActor::handleAppearance(SpawnEventType eventID, uint32_t eventArg)
{
    if(eventID == eSpawnEventAnimation)
    {
        SpawnAnimation anim = (SpawnAnimation)eventArg;
        switch(anim)
        {
        default:
        case eSpawnAnimInvalid:
            qDebug("Unhandled spawn animation 0x%x", anim);
            break;
        case eSpawnAnimStanding:
        case eSpawnAnimLooting:
        case eSpawnAnimSitting:
        case eSpawnAnimDucking:
        case eSpawnAnimFeigningDeath:
            m_spawnAnim = anim;
            break;
        }
    }
    else if(eventID == eSpawnEventDied)
    {
        // Prevent the corpse from moving due to movement prediction.
        SpawnState &cur = currentState();
        cur.innerVelocity = vec3(0.0f, 0.0f, 0.0f);
        cur.angular = 0.0f;
        if(m_info.type == eSpawnTypeNPC)
        {
            m_info.type = eSpawnTypeNPCCorpse;
        }
        else
        {
            m_info.type = eSpawnTypePCCorpse;
        }
        
    }
    else
    {
        qDebug("Spawn event: %d, arg: %d for spawn: %d", eventID, eventArg, spawnID());
    }
}

void CharacterActor::handleAnimation(uint8_t action, uint8_t value)
{
    CharacterAnimation animID = eAnimInvalid;
    switch(value)
    {
    case 3:
        animID = eAnim2Handed1;
        break;
    case 8:
        animID = eAnimAttackPrimary;
        break;
    }
    
    if(animID != eAnimInvalid)
    {
        m_queuedAnimationID = animID;
        return;
    }
    qDebug("Spawn animation: %d, val: %d for spawn: %d", action, value, spawnID());
}

void CharacterActor::updateHP(uint32_t currentHP, uint32_t maxHP)
{
    m_currentHP = currentHP;
    m_maxHP = maxHP;
}
