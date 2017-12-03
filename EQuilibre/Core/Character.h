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

#ifndef EQUILIBRE_CORE_CHARACTER_H
#define EQUILIBRE_CORE_CHARACTER_H

#include <QAbstractItemModel>
#include <QString>
#include <QVector>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/LinearMath.h"

enum GenderID
{
    eGenderMale = 0,
    eGenderFemale = 1,
    eGenderNeutral = 2
};

enum ClassID
{
    eClassInvalid = 0,
    eClassWarrior,
    eClassCleric,
    eClassPaladin,
    eClassRanger,
    eClassShadowKnight,
    eClassDruid,
    eClassMonk,
    eClassBard,
    eClassRogue,
    eClassShaman,
    eClassNecromancer,
    eClassWizard,
    eClassMagician,
    eClassEnchanter,
    eClassBeastLord,
    eClassBerserker
};

enum RaceID
{
    eRaceInvalid = 0,
#define RACE_ENTRY(id, name, modelMale, modelFemale, animMale, animFemale) eRace##name = id,
#include "EQuilibre/Core/Races.def"
#undef RACE_ENTRY
    eRaceLast
};

enum SlotID
{
    eSlotHead = 0,
    eSlotChest = 1,
    eSlotArms = 2,
    eSlotBracer = 3,
    eSlotHands = 4,
    eSlotLegs = 5,
    eSlotFeet = 6,
    eSlotPrimary = 7,
    eSlotSeconday = 8,
    eSlotCount = 9
};

struct  SpawnState
{
    vec3 position;
    float heading;
    vec3 innerVelocity;
    float angular;
    vec3 velocity;
    uint16_t speed;
    uint16_t sequence;
    
    SpawnState();
};

// Type of events specified by the SpawnAppearance message.
enum SpawnEventType
{
    eSpawnEventDied = 0,
    eSpawnEventWhoLevel = 1,
    eSpawnEventIsInvisible = 3,
    eSpawnEventPVP = 4,
    eSpawnEventLightEmitted = 5,
    eSpawnEventAnimation = 14,
    eSpawnEventIsSneaking = 15,
    eSpawnEventSetID = 16,
    eSpawnEventHPUpdate = 17,
    eSpawnEventIsLinkDead = 18,
    eSpawnEventIsLevitating = 19,
    eSpawnEventIsGM = 20,
    eSpawnEventIsAnonymous = 21,
    eSpawnEventGuildID = 22,
    eSpawnEventGuildRank = 23,
    eSpawnEventIsAFK = 24,
    eSpawnEventPetID = 25,
    eSpawnEventAutosplit = 28,
    eSpawnEventSize = 29,
    eSpawnEventNameColor = 31,
    eSpawnEventShowHelm = 43
};

enum SpawnAnimation
{
    eSpawnAnimInvalid = 0,
    eSpawnAnimStanding = 100,
    eSpawnAnimLooting = 105,
    eSpawnAnimSitting = 110,
    eSpawnAnimDucking = 111,
    eSpawnAnimFeigningDeath = 115
};

enum SpawnTypes
{
    eSpawnTypePC = 0,
    eSpawnTypeNPC = 1,
    eSpawnTypePCCorpse = 2,
    eSpawnTypeNPCCorpse = 3
};

enum BodyType
{
    eBodyTypeInvalid = 0,
    eBodyTypeHumanoid = 1,
    eBodyTypeLycanthrope = 2,
    eBodyTypeUndead = 3,
    eBodyTypeGiant = 4,
    eBodyTypeConstruct = 5,
    eBodyTypeExtraplanar = 6,
    eBodyTypeMagical = 7,
    eBodyTypeSummonedUndead = 8,
    eBodyTypeRaidGiant = 9,
    eBodyTypeNoTarget = 11,
    eBodyTypeVampire = 12,
    eBodyTypeAnimal = 21,
    eBodyTypeInsect = 22,
    eBodyTypeMonster = 23,
    eBodyTypeSummoned = 24,
    eBodyTypePlant = 25,
    eBodyTypeDragon = 26,
    eBodyTypeSummoned2 = 27,
    eBodyTypeSummoned3 = 28,
    eBodyTypeVeliousDragon = 30,
    eBodyTypeDragon3 = 32,
    eBodyTypeBoxes = 33,
    eBodyTypeNoTarget2 = 60,
    eBodyTypeInvisMan = 66,
    eBodyTypeSpecial = 67
};

struct  SpawnInfo
{
    SpawnInfo();
    QString name;
    uint32_t spawnID;
    SpawnState state;
    SpawnTypes type;
    ClassID classID;
    RaceID raceID;
    GenderID gender;
    BodyType bodyType;
    uint8_t level;
    uint8_t isGM;
    uint16_t deity;
    float scale;
    float walkSpeed;
    float runSpeed;
    uint32_t equip[eSlotCount];
    uint32_t equipColors[eSlotCount];
};

/** Contains information about a character race. */
struct  RaceInfo
{
    /** Network identifier for the race. Zero means invalid. */
    RaceID id;
    /** Name of the race. */
    const char *name;
    /** Model identifier for male characters or NULL. */
    const char *modelMale;
    /** Model identifier for female characters or NULL. */
    const char *modelFemale;
    /** Animation identifier for male characters or NULL. */
    const char *animMale;
    /** Animation identifier for female characters or NULL. */
    const char *animFemale;

    bool findByID(RaceID raceID);
    const char *genderName(GenderID gender);
    void clear();
};

enum CharacterAnimation
{
    eAnimInvalid = 0,
    eAnimBored,
    eAnimIdle,
    eAnimSit,
    eAnimRotate,
    eAnimRotate2,
    eAnimLoot,
    eAnimSwim,
    eAnimWalk,
    eAnimRun,
    eAnimSprint,
    eAnimJump,
    eAnimFall,
    eAnimCrouch,
    eAnimClimb,
    eAnimLoot2,
    eAnimSwim2,
    eAnimKick,
    eAnimPierce,
    eAnim2Handed1, // 3?
    eAnim2Handed2,
    eAnimAttackPrimary, // 8?
    eAnimAttackSecondary,
    eAnimAttack3,
    eAnimAttack4,
    eAnimArchery,
    eAnimAttackUnderwater,
    eAnimFlyingKick,
    eAnimDamageLow,
    eAnimDamageMed,
    eAnimDamageHigh,
    eAnimDeath,
    eAnimPlayString,
    eAnimPlayWind,
    eAnimSpell1,
    eAnimSpell2,
    eAnimSpell3,
    eAnimSpecialKick,
    eAnimSpecialPunch,
    eAnimSpecialBlow,
    eAnimEmoteCheer, // 27
    eAnimEmoteCry,   // 28
    eAnimEmoteWave,  // 29
    eAnimEmoteRude,  // 30
    // eAnimYawn 31
    // eAnimNod 48
    // eAnimApplaud 51
    // eAnimChuckle 54
    // eAnimWhistle 61
    // eAnimLaugh 63
    // eAnimTap 69
    // eAnimBow 70
    // eAnimSmile 77
    eAnimCount
};

struct  CharacterInfo
{
    CharacterInfo();
    QString name;
    uint32_t equip[eSlotCount];
    uint32_t equipColors[eSlotCount];
    uint32_t primaryID;
    uint32_t secondaryID;
    uint32_t zoneID;
    RaceID raceID;
    ClassID classID;
    GenderID gender;
    uint8_t level;
    uint8_t deityID;
    uint8_t hairStyle;
    uint8_t beard;
    uint8_t face;
    uint8_t hairColor;
    uint8_t beardColor;
    uint8_t eyeColor[2];
    bool goHomeAvailable;
    bool tutorialAvailable;
    
    const char * className() const;
    QString raceName() const;
    static const char * className(ClassID classID);
    static QString raceName(RaceID raceID);
    static const char * findAnimationName(CharacterAnimation animID);
    static CharacterAnimation findAnimationByName(const char *name);
};

class CharacterList : public QAbstractItemModel
{
public:
    CharacterList();
    QVector<CharacterInfo> & items();
    const QVector<CharacterInfo> & items() const;
    size_t size() const;
    
    const CharacterInfo *current() const;
    int currentIndex() const;
    void setCurrentIndex(int newIndex);
    bool setCurrentName(QString name);
    
    void append(const CharacterInfo &info);
    void clear();
    
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    
private:
    const CharacterInfo *toObject(QModelIndex index) const;
    
    QVector<CharacterInfo> m_items;
    int m_currentIndex;
};

#endif
