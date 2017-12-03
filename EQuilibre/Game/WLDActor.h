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

#ifndef EQUILIBRE_GAME_ACTOR_H
#define EQUILIBRE_GAME_ACTOR_H

#include <vector>
#include <QVector>
//#include "Newton.h"
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/Geometry.h"
#include "EQuilibre/Core/World.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/RenderContext.h"

class ActorFragment;
class CharacterActor;
class CharacterPack;
class Game;
class LightSourceFragment;
class ObjectPack;
class Octree;
class PFSArchive;
class WLDMesh;
class Zone;

/*!
  \brief Describes an instance of an entity that can be referenced by a spatial index.
  */
class  Actor
{
public:
    enum ActorType
    {
        Invalid = 0,
        Region,
        Object,
        Character,
        LightSource
    };
    
    Actor(ActorType type, Zone *zone = NULL);
    virtual ~Actor();

    ActorType type() const;
    const vec3 & location() const;
    const AABox & boundsAA() const;

    Zone * zone() const;
    
    template<typename T>
    T * cast()
    {
        if(m_type == T::Kind)
            return static_cast<T *>(this);
        else
            return 0;
    }
    
    template<typename T>
    const T * cast() const 
    {
        if(m_type == T::Kind)
            return static_cast<const T *>(this);
        else
            return 0;
    }
    
protected:

    Zone *m_zone;
    vec3 m_location;
    AABox m_boundsAA;
    ActorType m_type;
};

class  ObjectActor : public Actor
{
public:
    ObjectActor(ActorFragment *frag, WLDMesh *mesh, Zone *zone);
    ObjectActor(const ObjectInfo &info, WLDMesh *mesh, Zone *zone);
    ObjectActor(WLDMesh *mesh, Zone *zone);
    
    const static ActorType Kind = Object;
    
    WLDMesh * mesh() const;
    ObjectPack * pack() const;
    void setPack(ObjectPack *newPack);
    bool isGroundSpawn() const;
    bool isEquipment() const;
    const BufferSegment & colorSegment() const;
    
    bool isIndexed() const;
    void setIndexed(bool inIndex);
    void setLocation(const vec3 &newPos);
    const vec4 & rotation() const;
    void setRotation(const vec4 &newRot);
    const vec3 & scale() const;
    void setScale(const vec3 &newScale);
    const vec4 & innerRotation() const;
    void setInnerRotation(const vec4 &newRot);
    const vec3 & innerTranslation() const;
    void setInnerTranslation(const vec3 &newTrans);
    const matrix4 & modelMatrix() const;
    
    void importColorData(MeshBuffer *meshBuf);
    void update();
    
private:
    WLDMesh *m_mesh;
    ObjectPack *m_pack;
    bool m_groundSpawn;
    bool m_equip;
    bool m_inIndex;
    vec4 m_rotation;
    vec4 m_innerRotation;
    vec3 m_innerTranslation;
    vec3 m_scale;
    matrix4 m_modelMatrix;
    QVector<QRgb> m_colors;
    BufferSegment m_colorSegment;
};

class  RegionActor : public Actor
{
public:
    RegionActor(WLDMesh *mesh, uint32_t regionID, Zone *zone);
    
    const static ActorType Kind = Region;
    
    WLDMesh * mesh() const;
    
    uint32_t regionID() const;
    
    ZoneRegionType regionType() const;
    void setRegionType(ZoneRegionType newType);
    
    uint32_t zonePointID() const;
    void setZonePointID(uint32_t newID);
    
    std::vector<uint16_t> & nearbyRegions();
    const std::vector<uint16_t> & nearbyRegions() const;
    
    const std::vector<CharacterActor *> & characters() const;
    
    void addCharacter(CharacterActor *actor);
    void removeCharacter(CharacterActor *actor);

    
private:
    int findCharacter(CharacterActor *actor) const;
    
    WLDMesh *m_mesh;
    uint32_t m_regionID;
    ZoneRegionType m_regionType;
    uint32_t m_zonePointID;
    std::vector<uint16_t> m_nearbyRegions;
    std::vector<CharacterActor *> m_characters;
};

/*!
  \brief Describes an instance of a light source.
  */
class  LightActor : public Actor
{
public:
    LightActor(LightSourceFragment *frag, uint16_t lightID);
    const static ActorType Kind = LightSource;
   
    uint16_t lightID() const;
    const LightParams & params() const;
    
private:
    uint16_t m_lightID;
    LightParams m_params;
};

typedef void (*OctreeCallback)(Actor *actor, void *user);

class  OctreeIndex
{
public:
    OctreeIndex(AABox bounds, int maxDepth=5);
    ~OctreeIndex();
    
    Octree * add(Actor *actor);
    void findVisible(const Frustum &f, OctreeCallback callback, void *user, bool cull);
    void findVisible(const Sphere &s, OctreeCallback callback, void *user, bool cull);
    void findIdealInsertion(AABox bb, int &x, int &y, int &z, int &depth);
    Octree * findBestFittingOctant(int x, int y, int z, int depth);
    
private:
    void findVisible(const Frustum &f, Octree *octant, OctreeCallback callback, void *user, bool cull);
    void findVisible(const Sphere &f, Octree *octant, OctreeCallback callback, void *user, bool cull);
    
    Octree *m_root;
    int m_maxDepth;
};

class  Octree
{
public:
    Octree(AABox bounds, OctreeIndex *index);
    ~Octree();
    const AABox & strictBounds() const;
    AABox looseBounds() const;
    const QVector<Actor *> & actors() const;
    QVector<Actor *> & actors();
    Octree *child(int index) const;
    Octree *createChild(int index);
    
private:
    AABox m_bounds;
    OctreeIndex *m_index;
    Octree *m_children[8];
    QVector<Actor *> m_actors;
};

#endif
