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

#ifndef EQUILIBRE_GAME_CHARACTER_ACTOR_H
#define EQUILIBRE_GAME_CHARACTER_ACTOR_H

#include <QObject>
#include <QMap>
#include <QPair>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/Character.h"
#include "EQuilibre/Core/Geometry.h"
#include "EQuilibre/Core/Skeleton.h"
#include "EQuilibre/Game/WLDActor.h"

class Animation;
class CharacterPack;
class Game;
class GameClient;
struct GameUpdate;
class MaterialArray;
class MaterialMap;
class ObjectActor;
class RenderContext;
class RenderProgram;
class WLDMaterialPalette;
class WLDMaterialSlot;
class WLDMesh;
class CharacterModel;
class RegionActor;
class Zone;

enum BoneTrackID
{
    eTrackHead = 0,
    eTrackGuild,
    eTrackLeft,
    eTrackRight,
    eTrackShield,
    eTrackCount
};

/*!
  \brief Describes an instance of a character model.
  */
class CharacterActor : public Actor
{
public:
    CharacterActor(Zone *zone, bool player = false);
    virtual ~CharacterActor();
    const static ActorType Kind = Character;
    
    Game * game() const;
    Zone * zone() const;
    

    bool isPlayer() const;
    bool isCorpse() const;
    
    RegionActor * currentRegion() const;
    void setCurrentRegion(RegionActor *newRegion);
    
    CharacterPack * pack() const;
    void setPack(CharacterPack *newPack);
    
    CharacterModel * model() const;
    void setModel(CharacterModel *newModel);
    
    float capsuleHeight() const;
    float capsuleRadius() const;
    
    uint32_t spawnID() const;
    QString name() const;
    
    void setEquip(SlotID slotID, uint32_t value, uint32_t color);
    
    bool isWalking() const;
    void setWalking(bool isWalking);
    float speed() const;
    /* Multiplying speed by this gives the base velocity of the character. */
    float speedFactor() const;
    bool jumping() const;
    void setJumping(bool isJumping);
    
    const SpawnState & currentState() const;
    SpawnState & currentState();
    SpawnState & previousState();
    
    const SpawnInfo & info() const;
    void setInfo(const SpawnInfo &newInfo);    
    void setState(const SpawnState &newState);
    
    int currentHP() const;
    int maxHP() const;
    
    int movementStraight() const;
    int movementSideways() const;
    
    /** Angle describing where the character is faced. It is counter-clockwise:
      * north is 0.00, west is 0.25, south is 0.50 and east is 0.75. */
    float heading() const;
    void setHeading(float newHeading);
    
    vec3 tagPos() const;
    
    const matrix4 & modelMatrix() const;
    
    MaterialMap * materialMap() const;
    
    Animation * animation() const;
    void setAnimation(Animation *newAnim, CharacterAnimation newAnimID);
    
    uint32_t animationID();

    double animationTime() const;
    void setAnimationTime(double newTime);

    uint32_t skinID() const;
    void setSkinID(uint32_t newID);

    Animation * findAnimation(QString animName);
    void draw(RenderContext *renderCtx, RenderProgram *prog);
    void markEquipVisible();
    
    void move(const vec3 &newPosition, float newHeading);
    void handleAppearance(SpawnEventType eventID, uint32_t eventArg);
    void handleAnimation(uint8_t action, uint8_t value);
    void updateHP(uint32_t currentHP, uint32_t maxHP);
    
    void preMoveUpdate(const GameUpdate &gu);
    void postMoveUpdate(const GameUpdate &gu);
    void updateModelMatrix(const GameUpdate &gu);
    void updateAnimation(const GameUpdate &gu);
    void updateAnimationState(const GameUpdate &gu);
    
    void updatePosition(double dt);
    void handleCollisions();
    void interpolateState(double alpha);
    
    void jump();

private:
    static QString trackName(BoneTrackID trackID);
    void updateSkinTextures(WLDMaterialPalette *pal, uint32_t baseID);
    uint32_t skinIDForSlot(WLDMaterialSlot *slot, uint32_t baseID) const;
    void updateAnimSpeed();
    CharacterAnimation nextAnimation(bool &repeat, bool &lastFrame) const;
    void sendClientUpdate(const GameUpdate &gu);

    SpawnInfo m_info;
    QString m_name;
    bool m_inZone;
    uint32_t m_currentHP;
    uint32_t m_maxHP;
    RegionActor *m_currentRegion;
    float m_heading;
    matrix4 m_modelMatrix;
    matrix4 m_trackMatrix[eTrackCount];
    vec3 m_trackPos[eTrackCount];
    ObjectActor *m_trackObject[eTrackCount];
    int m_trackID[eTrackCount];
    float m_scale;
    bool m_walk;
    bool m_wantsToJump;
    bool m_jumping;
    bool m_player;
    /** If the actor is jumping, remaining duration of the jump. */
    float m_jumpTime;
    /** Movement state for the X axis.
      * -1: moving backward, 1: moving forward, 0: not moving. */
    int m_movementStraight;
    /** Movement state for the Y axis.
      * -1: moving right, 1: moving left, 0: not moving. */
    int m_movementSideways;
    // XXX Extend this to camera settings so that moving the camera isn't choppy.
    SpawnState m_previousState;
    SpawnState m_lastSentState;
    double m_lastUpdateTime;
    
    Game *m_game;
    CharacterPack *m_pack;
    CharacterModel *m_model;
    SpawnAnimation m_spawnAnim;
    Animation *m_animation;
    CharacterAnimation m_animationID;
    CharacterAnimation m_queuedAnimationID;
    bool m_repeatAnim;
    double m_startAnimationTime;
    double m_animTime;
    uint32_t m_skinID;
    MaterialMap *m_materialMap; // Slot ID -> Material ID in MaterialArray
    float m_capsuleHeight;
    float m_capsuleRadius;
};

#endif
