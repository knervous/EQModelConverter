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

#include <string.h>
#include "EQuilibre/Core/Character.h"
#include "EQuilibre/Core/Table.h"
#include "EQuilibre/Core/World.h"
#include "EQuilibre/Core/Races.def"

// This array contains all race identifiers in increasing order.
const static uint16_t IdentifierByIndex[RACE_COUNT] = {
#define RACE_ENTRY(id, name, modelMale, modelFemale, animMale, animFemale) \
    id,
#include "EQuilibre/Core/Races.def"
#undef RACE_ENTRY
};
    
// This array contains race info with the identifier matching the index.
const static RaceInfo Races[RACE_COUNT] = {
#define RACE_ENTRY(id, name, modelMale, modelFemale, animMale, animFemale) \
    {(RaceID)id, #name, modelMale, modelFemale, animMale, animFemale},
#include "EQuilibre/Core/Races.def"
#undef RACE_ENTRY
};

const static char * AnimNames[eAnimCount] = {
    "POS", // eAnimInvalid
    "O00", // eAnimBored
    "P01", // eAnimIdle
    "P02", // eAnimSit
    "P03", // eAnimRotate
    "P04", // eAnimRotate2
    "P05", // eAnimLoot
    "P06", // eAnimSwim
    "L01", // eAnimWalk
    "L02", // eAnimRun
    "L03", // eAnimSprint
    "L04", // eAnimJump
    "L05", // eAnimFall
    "L06", // eAnimCrouch
    "L07", // eAnimClimb
    "L08", // eAnimLoot2
    "L09", // eAnimSwim2
    "C01", // eAnimKick
    "C02", // eAnimPierce
    "C03", // eAnim2Handed1
    "C04", // eAnim2Handed2
    "C05", // eAnimAttackPrimary
    "C06", // eAnimAttackSecondary
    "C07", // eAnimAttack3
    "C08", // eAnimAttack4
    "C09", // eAnimArchery
    "C10", // eAnimAttackUnderwater
    "C11", // eAnimFlyingKick
    "D01", // eAnimDamageLow
    "D02", // eAnimDamageMed
    "D03", // eAnimDamageHigh
    "D05", // eAnimDeath
    "T02", // eAnimPlayString
    "T03", // eAnimPlayWind
    "T04", // eAnimSpell1
    "T05", // eAnimSpell2
    "T06", // eAnimSpell3
    "T07", // eAnimSpecialKick
    "T08", // eAnimSpecialPunch
    "T09", // eAnimSpecialBlow
    "S01", // eAnimEmoteCheer
    "S02", // eAnimEmoteCry
    "S03", // eAnimEmoteWave
    "S04", // eAnimEmoteRude
};

SpawnState::SpawnState()
{
    heading = angular = 0.0f;
    speed = 0;
    sequence = 0;
}

SpawnInfo::SpawnInfo()
{
    spawnID = 0;
    type = eSpawnTypePC;
    gender = eGenderMale;
    bodyType = eBodyTypeInvalid;
    isGM = 0;
    deity = 0;
    level = 0;
    classID = eClassInvalid;
    raceID = eRaceInvalid;
    scale = 1.0f;
    walkSpeed = 0.5f;
    runSpeed = 1.0f;
    for(int i = 0; i < eSlotCount; i++)
    {
        equip[i] = 0;
        equipColors[i] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////

CharacterInfo::CharacterInfo()
{
    for(int i = 0; i < eSlotCount; i++)
    {
        equip[i] = 0;
        equipColors[i] = 0;
    }
    primaryID = 0;
    secondaryID = 0;
    zoneID = 0;
    raceID = eRaceInvalid;
    classID = eClassInvalid;
    level = 0;
    gender = eGenderMale;
    deityID = 0;
    hairStyle = 0;
    beard = 0;
    face = 0;
    hairColor = 0;
    beardColor = 0;
    eyeColor[0] = 0;
    eyeColor[1] = 0;
    goHomeAvailable = false;
    tutorialAvailable = false;
}

const char * CharacterInfo::className() const
{
    return className(classID);
}

QString CharacterInfo::raceName() const
{
    return raceName(raceID);
}

const char * CharacterInfo::className(ClassID classID)
{
    switch(classID)
    {
    default:
        return "Unknown";
    case eClassWarrior:
        return "Warrior";
    case eClassCleric:
        return "Cleric";
    case eClassPaladin:
        return "Paladin";
    case eClassRanger:
        return "Ranger";
    case eClassShadowKnight:
        return "Shadowknight";
    case eClassDruid:
        return "Druid";
    case eClassMonk:
        return "Monk";
    case eClassBard:
        return "Bard";
    case eClassRogue:
        return "Rogue";
    case eClassShaman:
        return "Shaman";
    case eClassNecromancer:
        return "Necromancer";
    case eClassWizard:
        return "Wizard";
    case eClassMagician:
        return "Magician";
    case eClassEnchanter:
        return "Enchanter";
    case eClassBeastLord:
        return "Beastlord";
    case eClassBerserker:
        return "Berserker";
    }
}

QString CharacterInfo::raceName(RaceID raceID)
{
    QString race = "Unknown";
    RaceInfo info;
    if(info.findByID(raceID))
        race = info.name;
    return race.replace("_", " ");
}

const char * CharacterInfo::findAnimationName(CharacterAnimation animID)
{
    if(animID >= eAnimCount)
    {
        return NULL;
    }
    return AnimNames[animID];
}

CharacterAnimation CharacterInfo::findAnimationByName(const char *name)
{
    for(unsigned i = 0; i < eAnimCount; i++)
    {
        if(strcmp(AnimNames[i], name) == 0)
        {
            return (CharacterAnimation)i;
        }
    }
    return eAnimCount;
}

////////////////////////////////////////////////////////////////////////////////

void RaceInfo::clear()
{
    id = eRaceInvalid;
    name = NULL;
    modelMale = modelFemale = NULL;
    animMale = animFemale = NULL;
}

bool RaceInfo::findByID(RaceID raceID)
{
    int index = findTableEntry((uint32_t)raceID, IdentifierByIndex, RACE_COUNT);
    if(index < 0)
        return false;
    *this = Races[index];
    return true;
}

const char * RaceInfo::genderName(GenderID gender)
{
    if(modelMale && modelFemale)
    {
        switch(gender)
        {
        case eGenderMale:
            return "Male";
        case eGenderFemale:
            return "Female";
        }
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

CharacterList::CharacterList() : QAbstractItemModel()
{
    m_currentIndex = -1;
}

QVector<CharacterInfo> & CharacterList::items()
{
    return m_items;
}

const QVector<CharacterInfo> & CharacterList::items() const
{
    return m_items;
}

size_t CharacterList::size() const
{
    return m_items.size();
}

const CharacterInfo * CharacterList::current() const
{
    if((m_currentIndex < 0) || (m_currentIndex >= m_items.size()))
        return NULL;
    return &m_items[m_currentIndex];
}

int CharacterList::currentIndex() const
{
    return m_currentIndex;
}

void CharacterList::setCurrentIndex(int newIndex)
{
    m_currentIndex = newIndex;
}

bool CharacterList::setCurrentName(QString name)
{
    for(uint32_t i = 0; i < m_items.size(); i++)
    {
        const CharacterInfo &character = m_items[i];
        if(character.name == name)
        {
            m_currentIndex = i;
            return true;
        }
    }
    return false;
}

void CharacterList::append(const CharacterInfo &info)
{
    int index = m_items.count();
    beginInsertRows(QModelIndex(), index, index);
    m_items.append(info);
    endInsertRows();
}

void CharacterList::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

const CharacterInfo * CharacterList::toObject(QModelIndex index) const
{
    if(!index.isValid() || (index.row() < 0) || (index.row() >= rowCount()))
        return NULL;
    else
        return &m_items[index.row()];
}

QModelIndex CharacterList::index(int row, int column, const QModelIndex &parent) const
{
    if(parent.isValid() || (row < 0) || (row >= rowCount()))
        return QModelIndex();
    else
        return createIndex(row, column);
}

QModelIndex CharacterList::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int CharacterList::rowCount(const QModelIndex &parent) const
{
    return m_items.count();
}

int CharacterList::columnCount(const QModelIndex &parent) const
{
    return 5;
}

static QString zoneName(uint16_t zoneID)
{
    ZoneInfo zone;
    if(zone.findByID(zoneID))
    {
        return zone.longName;
    }
    return QString("Zone #%1").arg(zoneID);
}

QVariant CharacterList::data(const QModelIndex &index, int role) const
{
    const CharacterInfo *character = toObject(index);
    if(!character)
        return QVariant();
    else if((role == Qt::DisplayRole) || (role == Qt::ToolTipRole))
    {
        switch(index.column())
        {
        case 0:
            return character->name;
        case 1:
            return character->className();
        case 2:
            return character->raceName();
        case 3:
            return character->level;
        case 4:
            return zoneName((uint16_t)character->zoneID);
        }
    }
    return QVariant();
}

QVariant CharacterList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if((role == Qt::DisplayRole) && (orientation == Qt::Horizontal))
    {
        switch(section)
        {
        case 0:
            return "Name";
        case 1:
            return "Class";
        case 2:
            return "Race";
        case 3:
            return "Level";
        case 4:
            return "Zone";
        }
    }
    return QVariant();
}

Qt::ItemFlags CharacterList::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

