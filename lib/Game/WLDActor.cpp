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
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/ZoneTerrain.h"
#include "EQuilibre/Core/Fragments.h"
#include "EQuilibre/Render/Material.h"

Actor::Actor(ActorType type, Zone *zone)
{
    m_type = type;
    m_zone = zone;
    //m_shape = NULL;
}

Actor::~Actor()
{
    //clearShape();
}

const vec3 & Actor::location() const
{
    return m_location;
}

const AABox & Actor::boundsAA() const
{
    return m_boundsAA;
}

Actor::ActorType Actor::type() const
{
    return m_type;
}


Zone * Actor::zone() const
{
    return m_zone;
}


////////////////////////////////////////////////////////////////////////////////

RegionActor::RegionActor(WLDMesh *mesh, uint32_t regionID, Zone *zone) : Actor(Kind, zone)
{
    m_regionType = eRegionNormal;
    m_regionID = regionID;
    m_zonePointID = 0;
    m_mesh = mesh;
    m_boundsAA = mesh->boundsAA();
}

WLDMesh * RegionActor::mesh() const
{
    return m_mesh;
}

uint32_t RegionActor::regionID() const
{
    return m_regionID;
}

ZoneRegionType RegionActor::regionType() const
{
    return m_regionType;
}

void RegionActor::setRegionType(ZoneRegionType newType)
{
    m_regionType = newType;
}

uint32_t RegionActor::zonePointID() const
{
    return m_zonePointID;
}

void RegionActor::setZonePointID(uint32_t newID)
{
    m_zonePointID = newID;
}

std::vector<uint16_t> & RegionActor::nearbyRegions() 
{
    return m_nearbyRegions;
}

const std::vector<uint16_t> & RegionActor::nearbyRegions() const
{
    return m_nearbyRegions;
}

const std::vector<CharacterActor *> & RegionActor::characters() const
{
    return m_characters;
}

int RegionActor::findCharacter(CharacterActor *actor) const
{
    for(int i = 0; i < m_characters.size(); i++)
    {
        if(m_characters[i] == actor)
            return i;
    }
    return -1;
}

void RegionActor::addCharacter(CharacterActor *actor)
{
    int index = findCharacter(actor);
    if(index < 0)
        m_characters.push_back(actor);
}

void RegionActor::removeCharacter(CharacterActor *actor)
{
    int index = findCharacter(actor);
    if(index >= 0)
        m_characters.erase(m_characters.begin() + index);
}


////////////////////////////////////////////////////////////////////////////////

ObjectActor::ObjectActor(ActorFragment *frag, WLDMesh *mesh, Zone *zone) : Actor(Kind, zone)
{
    Q_ASSERT(frag);
    m_mesh = mesh;
    m_pack = NULL;
    m_groundSpawn = false;
    m_equip = false;
    m_inIndex = false;
    m_location = frag->m_location;
    m_rotation = vec4::quatFromEuler(frag->m_rotation);
    m_innerRotation = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_innerTranslation = vec3(0.0f, 0.0f, 0.0f);
    m_scale = frag->m_scale;
    
    // XXX Do object instances share the same baked lighting?
    if(frag->m_lighting && frag->m_lighting->m_def)
    {
        m_colors = frag->m_lighting->m_def->m_colors;
    }
    update();
}

ObjectActor::ObjectActor(const ObjectInfo &info, WLDMesh *mesh, Zone *zone) : Actor(Kind, zone)
{
    m_mesh = mesh;
    m_pack = NULL;
    m_groundSpawn = true;
    m_equip = false;
    m_inIndex = false;
    m_location = info.pos;
    m_rotation = vec4::quatFromEuler(vec3(0.0f, 0.0f, info.heading));
    m_innerRotation = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_innerTranslation = vec3(0.0f, 0.0f, 0.0f);
    m_scale = vec3(1.0f, 1.0f, 1.0f);
    update();
}

ObjectActor::ObjectActor(WLDMesh *mesh, Zone *zone) : Actor(Kind, zone)
{
    m_mesh = mesh;
    m_pack = NULL;
    m_groundSpawn = false;
    m_equip = true;
    m_inIndex = false;
    m_location = vec3(0.0f, 0.0f, 0.0f);
    m_rotation = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_innerRotation = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_innerTranslation = vec3(0.0f, 0.0f, 0.0f);
    m_scale = vec3(1.0f, 1.0f, 1.0f);
    update();
}

WLDMesh * ObjectActor::mesh() const
{
    return m_mesh;
}

ObjectPack * ObjectActor::pack() const
{
    return m_pack;
}

void ObjectActor::setPack(ObjectPack *newPack)
{
    m_pack = newPack;
}

bool ObjectActor::isGroundSpawn() const
{
    return m_groundSpawn;
}

bool ObjectActor::isEquipment() const
{
    return m_equip;
}

const BufferSegment & ObjectActor::colorSegment() const
{
    return m_colorSegment;
}

bool ObjectActor::isIndexed() const
{
    return m_inIndex;
}

void ObjectActor::setIndexed(bool inIndex)
{
    m_inIndex = inIndex;
}

void ObjectActor::setLocation(const vec3 &newPos)
{
    m_location = newPos;
}

const vec4 & ObjectActor::rotation() const
{
    return m_rotation;
}

void ObjectActor::setRotation(const vec4 &newRot)
{
    m_rotation = newRot;
}

const vec3 & ObjectActor::scale() const
{
    return m_scale;
}

void ObjectActor::setScale(const vec3 &newScale)
{
    m_scale = newScale;
}

const vec4 & ObjectActor::innerRotation() const
{
    return m_innerRotation;
}

void ObjectActor::setInnerRotation(const vec4 &newRot)
{
    m_innerRotation = newRot;
}

const vec3 & ObjectActor::innerTranslation() const
{
    return m_innerTranslation;
}

void ObjectActor::setInnerTranslation(const vec3 &newTrans)
{
    m_innerTranslation = newTrans;
}

const matrix4 & ObjectActor::modelMatrix() const
{
    return m_modelMatrix;
}

void ObjectActor::update()
{
    m_boundsAA = m_mesh->boundsAA();
    m_boundsAA.rotate(m_innerRotation);
    m_boundsAA.translate(m_innerTranslation);
    m_boundsAA.scale(m_scale);
    m_boundsAA.rotate(m_rotation);
    m_boundsAA.translate(m_location);
    
    m_modelMatrix = matrix4::translate(m_location);
    m_modelMatrix = m_modelMatrix * matrix4::rotate(m_rotation);
    m_modelMatrix = m_modelMatrix * matrix4::scale(m_scale.x, m_scale.y, m_scale.z);
    m_modelMatrix = m_modelMatrix * matrix4::translate(m_innerTranslation);
    m_modelMatrix = m_modelMatrix * matrix4::rotate(m_innerRotation);
}

void ObjectActor::importColorData(MeshBuffer *meshBuf)
{
    m_colorSegment.offset = meshBuf->colors.count();
    m_colorSegment.count = m_colors.count();
    m_colorSegment.elementSize = sizeof(uint32_t);
    for(int i = 0; i < m_colorSegment.count; i++)
        meshBuf->colors.append(m_colors[i]);
}

////////////////////////////////////////////////////////////////////////////////

LightActor::LightActor(LightSourceFragment *frag, uint16_t lightID) : Actor(Kind)
{
    m_lightID = lightID;
    if(frag)
    {
        vec3 radius(frag->m_radius, frag->m_radius, frag->m_radius);
        m_boundsAA.low = frag->m_pos - radius;
        m_boundsAA.high = frag->m_pos + radius;
        m_location = frag->m_pos;
        
        LightDefFragment *def = frag->m_ref->m_def;
        m_params.color = vec3(def->m_color.x, def->m_color.y, def->m_color.z);
        m_params.bounds.pos = frag->m_pos;
        m_params.bounds.radius = frag->m_radius;
    }
    else
    {
        m_params.color = vec3(0.0, 0.0, 0.0);
        m_params.bounds.pos = vec3(0.0, 0.0, 0.0);
        m_params.bounds.radius = 0.0;
    }
}

uint16_t LightActor::lightID() const
{
    return m_lightID;
}

const LightParams & LightActor::params() const
{
    return m_params;
}

////////////////////////////////////////////////////////////////////////////////

OctreeIndex::OctreeIndex(AABox bounds, int maxDepth)
{
    // Convert the bounds to a cube.
    float cubeLow = qMin(bounds.low.x, qMin(bounds.low.y, bounds.low.z));
    float cubeHigh = qMax(bounds.high.x, qMax(bounds.high.y, bounds.high.z));
    AABox cubeBounds(vec3(cubeLow, cubeLow, cubeLow), vec3(cubeHigh, cubeHigh, cubeHigh));
    m_root = new Octree(cubeBounds, this);
    m_maxDepth = maxDepth;
}

OctreeIndex::~OctreeIndex()
{
    delete m_root;
}

Octree * OctreeIndex::add(Actor *actor)
{
    AABox actorBounds = actor->boundsAA();
    int x = 0, y = 0, z = 0, depth = 0;
    findIdealInsertion(actorBounds, x, y, z, depth);
    Octree *octant = findBestFittingOctant(x, y, z, depth);
#ifndef NDEBUG
    vec3 actorCenter = actorBounds.center();
    AABox octantSBounds = octant->strictBounds();
    AABox octantLBounds = octant->looseBounds();
    Q_ASSERT(octantSBounds.contains(actorCenter));
    Q_ASSERT(octantLBounds.contains(actorBounds));
#endif
    octant->actors().append(actor);
    return octant;
}

void OctreeIndex::findVisible(const Frustum &f, OctreeCallback callback, void *user, bool cull)
{
    findVisible(f, m_root, callback, user, cull);
}

void OctreeIndex::findVisible(const Frustum &f, Octree *octant, OctreeCallback callback, void *user, bool cull)
{
    if(!octant)
        return;
    TestResult r = cull ? f.containsAABox(octant->looseBounds()) : INSIDE;
    if(r == OUTSIDE)
        return;
    cull = (r != INSIDE);
    for(int i = 0; i < 8; i++)
        findVisible(f, octant->child(i), callback, user, cull);
    foreach(Actor *actor, octant->actors())
    {
        if((r == INSIDE) || (f.containsAABox(actor->boundsAA()) != OUTSIDE))
            (*callback)(actor, user);
    }
}

void OctreeIndex::findVisible(const Sphere &s, OctreeCallback callback, void *user, bool cull)
{
    findVisible(s, m_root, callback, user, cull);
}

void OctreeIndex::findVisible(const Sphere &s, Octree *octant, OctreeCallback callback, void *user, bool cull)
{
    if(!octant)
        return;
    TestResult r = cull ? s.containsAABox(octant->looseBounds()) : INSIDE;
    if(r == OUTSIDE)
        return;
    cull = (r != INSIDE);
    for(int i = 0; i < 8; i++)
        findVisible(s, octant->child(i), callback, user, cull);
    foreach(Actor *actor, octant->actors())
    {
        if((r == INSIDE) || (s.containsAABox(actor->boundsAA()) != OUTSIDE))
            (*callback)(actor, user);
    }
}

void OctreeIndex::findIdealInsertion(AABox bb, int &x, int &y, int &z, int &depth)
{
    // Determine the maximum depth at which the bounds fit the octant.
    AABox sb = m_root->strictBounds();
    float sbRadius = (sb.high.x - sb.low.x);
    vec3 bbExtent = (bb.high - bb.low) * 0.5f;
    float bbRadius = qMax(bbExtent.x, qMax(bbExtent.y, bbExtent.z));
    depth = 0;
    while(true)
    {
        sbRadius *= 0.5f;
        if(sbRadius < bbRadius)
            break;
        depth++;
    }
    depth--;
    sbRadius *= 2.0f;
    
    // Get the index of the node at this depth.
    vec3 bbCenter = bb.center() - sb.low;
    vec3 sbSize = (sb.high - sb.low);
    int scale = 1 << depth;
    x = (int)floor((scale * bbCenter.x) / sbSize.x);
    y = (int)floor((scale * bbCenter.y) / sbSize.y);
    z = (int)floor((scale * bbCenter.z) / sbSize.z);
    Q_ASSERT(x >= 0 && x < scale);
    Q_ASSERT(y >= 0 && y < scale);
    Q_ASSERT(z >= 0 && z < scale);
}

Octree * OctreeIndex::findBestFittingOctant(int x, int y, int z, int depth)
{
    Octree *octant = m_root;
    int highBit = 1 << (depth - 1);
    for(int i = 0; i < qMin(depth, m_maxDepth); i++)
    {
        // Determine the octant at this depth using the index's current high bit.
        int localX = (x & highBit) > 0;
        int localY = (y & highBit) > 0;
        int localZ = (z & highBit) > 0;
        int childIndex = localX + (localY << 1) + (localZ << 2);
        Q_ASSERT(childIndex >= 0 && childIndex <= 7);
        Octree *newOctant = octant->child(childIndex);
        if(!newOctant)
        {
            // Create children octants as needed.
            newOctant = octant->createChild(childIndex);
            Q_ASSERT(newOctant);
        }
        octant = newOctant;
        highBit >>= 1;
    }
    return octant;
}

////////////////////////////////////////////////////////////////////////////////

Octree::Octree(AABox bounds, OctreeIndex *index)
{
    m_bounds = bounds;
    m_index = index;
    for(int i = 0; i < 8; i++)
        m_children[i] = NULL;
}

Octree::~Octree()
{
    for(int i = 0; i < 8; i++)
        delete m_children[i];
}

const AABox & Octree::strictBounds() const
{
    return m_bounds;
}

AABox Octree::looseBounds() const
{
    AABox loose = m_bounds;
    loose.scaleCenter(2.0f);
    return loose;
}

const QVector<Actor *> & Octree::actors() const
{
    return m_actors;
}

QVector<Actor *> & Octree::actors()
{
    return m_actors;
}

Octree *Octree::child(int index) const
{
    return ((index >= 0) && (index < 8)) ? m_children[index] : NULL;
}

Octree * Octree::createChild(int index)
{
    Octree *octant = NULL;
    vec3 l = m_bounds.low, c = m_bounds.center(), h = m_bounds.high;
    switch(index)
    {
    case 0: // x = 0, y = 0, z = 0
        octant = new Octree(AABox(vec3(l.x, l.y, l.z), vec3(c.x, c.y, c.z)), m_index);
        break;
    case 1: // x = 1, y = 0, z = 0
        octant = new Octree(AABox(vec3(c.x, l.y, l.z), vec3(h.x, c.y, c.z)), m_index);
        break;
    case 2: // x = 0, y = 1, z = 0
        octant = new Octree(AABox(vec3(l.x, c.y, l.z), vec3(c.x, h.y, c.z)), m_index);
        break;
    case 3: // x = 1, y = 1, z = 0
        octant = new Octree(AABox(vec3(c.x, c.y, l.z), vec3(h.x, h.y, c.z)), m_index);
        break;
    case 4: // x = 0, y = 0, z = 1
        octant = new Octree(AABox(vec3(l.x, l.y, c.z), vec3(c.x, c.y, h.z)), m_index);
        break;
    case 5: // x = 1, y = 0, z = 1
        octant = new Octree(AABox(vec3(c.x, l.y, c.z), vec3(h.x, c.y, h.z)), m_index);
        break;
    case 6: // x = 0, y = 1, z = 1
        octant = new Octree(AABox(vec3(l.x, c.y, c.z), vec3(c.x, h.y, h.z)), m_index);
        break;
    case 7: // x = 1, y = 1, z = 1
        octant = new Octree(AABox(vec3(c.x, c.y, c.z), vec3(h.x, h.y, h.z)), m_index);
        break;
    }
    if(octant)
        m_children[index] = octant;
    return octant;
}
