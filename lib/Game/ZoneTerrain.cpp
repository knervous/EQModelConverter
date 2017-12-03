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
#include "EQuilibre/Game/ZoneTerrain.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Core/Fragments.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderProgram.h"

ZoneTerrain::ZoneTerrain(Zone *zone)
{
    m_zone = zone;
    m_regionCount = 0;
    m_currentRegion = 0;
    m_uploadStart = 0.0;
    m_state = eAssetNotLoaded;
    m_regionTree = NULL;
    m_zoneBuffer = NULL;
    m_uploadFence = NULL;
    m_palette = NULL;
    m_batch = NULL;
    m_zoneStat = NULL;
    m_zoneStatGPU = NULL;
}

ZoneTerrain::~ZoneTerrain()
{
    unload();
}

AssetLoadState ZoneTerrain::state() const
{
    return m_state;
}

const AABox & ZoneTerrain::bounds() const
{
    return m_zoneBounds;
}

uint32_t ZoneTerrain::currentRegionID() const
{
    return m_currentRegion;
}

void ZoneTerrain::setCurrentRegionID(uint32_t newID)
{
    m_currentRegion = newID;
}

const std::vector<RegionActor *> & ZoneTerrain::visibleRegions() const
{
    return m_visibleRegions;
}

void ZoneTerrain::unload()
{
    Game *game = m_zone->game();
    for(uint32_t i = 1; i <= m_regionCount; i++)
    {
        RegionActor *part = m_regionActors[i];
        if(part)
        {
            delete part->mesh();
            delete part;
        }
    }
    m_regionActors.clear();
    m_visibleRegions.clear();
    m_regionCount = 0;
    m_currentRegion = 0;
    m_regionTree = NULL;
    delete m_batch;
    m_batch = NULL;
    game->clearFence(m_uploadFence);
    game->clearBuffer(m_zoneBuffer);
    if(m_palette)
    {
        MaterialArray *matArray = m_palette->array();
        game->clearMaterials(matArray);
        delete m_palette;
        m_palette = NULL;    
    }
    m_state = eAssetNotLoaded;
}

bool ZoneTerrain::load(PFSArchive *archive, WLDData *wld)
{
    if(!archive || !wld)
        return false;
    
    // Load region info.
    WLDFragmentArray<RegionTreeFragment> regionTrees = wld->table()->byKind<RegionTreeFragment>();
    if(regionTrees.count() != 1)
        return false;
    m_regionTree = regionTrees[0];
    WLDFragmentArray<RegionFragment> regionDefs = wld->table()->byKind<RegionFragment>();
    if(regionDefs.count() == 0)
        return false;
    m_regionCount = regionDefs.count();
    m_regionActors.resize(m_regionCount + 1, NULL);
    m_visibleRegions.reserve(m_regionCount);
    
    // Load zone regions as model parts, computing the zone's bounding box.
    m_zoneBounds = AABox();
    WLDFragmentArray<MeshDefFragment> meshDefs = wld->table()->byKind<MeshDefFragment>();
    for(uint32_t i = 0; i < meshDefs.count(); i++)
    {
        MeshDefFragment *meshDef = meshDefs[i];
        QString name = meshDef->name();
        int typePos = name.indexOf("_DMSPRITEDEF");
        if((name.length() < 2) || !name.startsWith("R") || (typePos < 0))
            continue;
        bool ok = false;
        uint32_t regionID = (uint32_t)name.mid(1, typePos - 1).toInt(&ok);
        if(!ok || (regionID >= m_regionCount))
            continue;
        WLDMesh *meshPart = new WLDMesh(meshDef, regionID);
        m_zoneBounds.extendTo(meshPart->boundsAA());
        RegionActor *actor = new RegionActor(meshPart, regionID, m_zone);
        if(regionID <= regionDefs.count())
        {
            RegionFragment *region = regionDefs[regionID - 1];
            std::vector<uint16_t> &nearby = actor->nearbyRegions();
            nearby.insert(nearby.end(), region->m_nearbyRegions.begin(),
                          region->m_nearbyRegions.end());
        }
        m_regionActors[regionID] = actor;
    }
    vec3 padding(1.0, 1.0, 1.0);
    m_zoneBounds.low = m_zoneBounds.low - padding;
    m_zoneBounds.high = m_zoneBounds.high + padding;
//    NewtonSetWorldSize(m_zone->collisionWorld(),
//                       (float *)&m_zoneBounds.low,
//                       (float *)&m_zoneBounds.high);
    
    // Load region type info.
    QRegExp zonePointExp("^DRNTP(\\d{5})(\\d{6})_ZONE$");
    WLDFragmentArray<RegionTypeFragment> regionTypes = wld->table()->byKind<RegionTypeFragment>();
    for(uint32_t i = 0; i < regionTypes.count(); i++)
    {
        RegionTypeFragment *regionType = regionTypes[i];
        QString typeName = regionType->name();
        ZoneRegionType type = eRegionNormal;
        uint32_t zonePointID = 0;
        if(typeName == "WT_ZONE")
        {
            type = eRegionUnderwater;
        }
        else if(typeName == "LA_ZONE")
        {
            type = eRegionLava;
        }
        else if(typeName == "DRP_ZONE")
        {
            type = eRegionPvP;
        }
        else if(zonePointExp.exactMatch(typeName))
        {
            type = eRegionZonePoint;
            zonePointID = zonePointExp.cap(2).toInt();
        }
        else
        {
            qDebug("Unknown region type: %s", typeName.toLatin1().constData());
        }
        
        QVector<uint32_t> &regions = regionType->m_regions;
        for(uint32_t i = 0; i < regions.count(); i++)
        {
            // Region indices start at zero, but region IDs start at one.
            uint32_t regionID = (regions[i] + 1);
            RegionActor *actor = regionActor(regionID);
            if(actor)
            {
                actor->setRegionType(type);
                actor->setZonePointID(zonePointID);
            }
        }
    }
    
    // Load zone textures into the material palette.
    WLDFragmentArray<MaterialPaletteFragment> matPals = wld->table()->byKind<MaterialPaletteFragment>();
    Q_ASSERT(matPals.count() == 1);
    m_palette = new WLDMaterialPalette(archive);
    m_palette->setDef(matPals[0]);
    m_palette->createSlots();
    m_palette->createArray();
    m_palette->createMap();
    
    // Import vertices and indices for each mesh.
    m_zoneBuffer = new MeshBuffer();
    for(uint32_t i = 1; i <= m_regionCount; i++)
    {
        RegionActor *actor = m_regionActors[i];
        if(actor)
            actor->mesh()->importFrom(m_zoneBuffer);
    }
    m_state = eAssetLoaded;
    return true;
}

void ZoneTerrain::showAllRegions(const Frustum &frustum)
{
    for(uint32_t i = 0; i < m_regionCount; i++)
    {
        RegionActor *actor = m_regionActors[i];
        if(actor)
        {
            if(frustum.containsAABox(actor->boundsAA()) != OUTSIDE)
                m_visibleRegions.push_back(actor);
        }
    }
}

void ZoneTerrain::showNearbyRegions(const Frustum &frustum)
{
    RegionActor *region = regionActor(m_currentRegion);
    if(!region)
    {
        return;
    }
    
    const std::vector<uint16_t> &nearbyRegions = region->nearbyRegions();
    for(uint32_t i = 0; i < nearbyRegions.size(); i++)
    {
        uint32_t nearbyRegionID = nearbyRegions[i];
        RegionActor *nearbyRegion = regionActor(nearbyRegionID);
        if(nearbyRegion)
        {
            if(frustum.containsAABox(nearbyRegion->boundsAA()) != OUTSIDE)
                m_visibleRegions.push_back(nearbyRegion);
        }
    }
}

void ZoneTerrain::showCurrentRegion(const Frustum &frustum)
{
    if(m_currentRegion == 0)
        return;
    RegionActor *actor = regionActor(m_currentRegion);
    if(actor)
    {
        if(frustum.containsAABox(actor->boundsAA()) != OUTSIDE)
            m_visibleRegions.push_back(actor);
    }
}

RegionActor * ZoneTerrain::findRegionActor(const vec3 &pos)
{
    uint32_t id = findRegionID(pos);
    return regionActor(id);
}

uint32_t ZoneTerrain::findRegionID(const vec3 &pos) const
{
    return findRegion(pos, m_regionTree->m_nodes.constData(), 1);
}

RegionActor * ZoneTerrain::regionActor(uint32_t regionID) const
{
    if(!regionID || (regionID > m_regionActors.size()))
        return NULL;
    return m_regionActors[regionID];
}

uint32_t ZoneTerrain::findRegion(const vec3 &pos, const RegionTreeNode *nodes, uint32_t nodeIdx) const
{
    if(nodeIdx == 0)
        return 0;
    const RegionTreeNode &node = nodes[nodeIdx-1];
    if(node.regionID == 0)
    {
        float distance = vec3::dot(node.normal, pos) + node.distance;
        return findRegion(pos, nodes, (distance >= 0.0f) ? node.left : node.right);
    }
    else
    {
        // Leaf node.
        return node.regionID;
    }
}

/**
 * @brief Add all regions that intersect the sphere to the region list.
 *
 * @param sphere Sphere to intersect.
 * @param regions Array to copy the intersecting regions to.
 * @param maxRegions Maximum number of regions to return in @ref regions.
 * @return Number of intersecting regions copied to @ref regions.
 */
uint32_t ZoneTerrain::findRegions(Sphere sphere, uint32_t *regions, uint32_t maxRegions)
{
    uint32_t found = 0;
    findRegions(sphere, m_regionTree->m_nodes.constData(), 1, regions, maxRegions, found);
    return found;
}

/**
 * @brief Add all regions that intersect the sphere to the region list.
 *
 * @param sphere Sphere to intersect.
 * @param nodes List of nodes that make the region BSP tree.
 * @param nodeIdx Current node to test for intersection.
 * @param regions Array to copy the intersecting regions to.
 * @param maxRegions Maximum number of regions to return in @ref regions.
 * @param found Number of intersection regions found so far.
 */
void ZoneTerrain::findRegions(const Sphere &sphere,
                              const RegionTreeNode *nodes,
                              uint32_t nodeIdx,
                              uint32_t *regions,
                              uint32_t maxRegions, uint32_t &found)
{
    if((nodeIdx == 0) || (maxRegions == found))
        return;
    const RegionTreeNode &node = nodes[nodeIdx-1];
    if(node.regionID == 0)
    {
        float distance = vec3::dot(node.normal, sphere.pos) + node.distance;
        if((distance >= 0.0f) || (sphere.radius >= -distance))
            findRegions(sphere, nodes, node.left, regions, maxRegions, found);
        if((distance <= 0.0f) || (sphere.radius >= distance))
            findRegions(sphere, nodes, node.right, regions, maxRegions, found);
    }
    else
    {
        // Leaf node.
        regions[found++] = node.regionID;
    }
}

void ZoneTerrain::resetVisible()
{
    m_visibleRegions.clear();
}

bool ZoneTerrain::upload()
{
    Game *game = m_zone->game();
    RenderContext *renderCtx = game->renderContext();
    if(!renderCtx || !renderCtx->isValid())
    {
        return false;
    }
    
    MaterialArray *materials = m_palette->array();
    if(m_state == eAssetLoaded)
    {
        // Start uploading the textures and update the material subtextures.
        m_uploadStart = game->currentTime();
        materials->uploadArray(renderCtx, false);
        m_state = eAssetUploadingTextures;
        
        for(uint32_t i = 1; i <= m_regionCount; i++)
        {
            RegionActor *actor = m_regionActors[i];
            if(actor)
            {
                MeshData *meshData = actor->mesh()->data();
                meshData->updateTexCoords(materials, true);
            }
        }
        m_batch = new RenderBatch();
        
        // Create the GPU buffers and free the memory used for vertices and indices.
        m_zoneBuffer->upload(renderCtx);
        m_zoneBuffer->clearVertices();
        m_zoneBuffer->clearIndices();
        
        // Create a fence to know when uploading is done.
        m_uploadFence = renderCtx->createFence();
    }
    
    // Check for texture upload completion.
    if((m_state == eAssetUploadingTextures) &&
       (materials->checkUpload(renderCtx) == eUploadFinished) &&
       renderCtx->isFenceSignaled(m_uploadFence))
    {
        m_state = eAssetUploaded;
        renderCtx->deleteFence(m_uploadFence);
        m_uploadFence = NULL;
        
        double uploadDuration = game->currentTime() - m_uploadStart;
        qDebug("Uploaded terrain in %f.", uploadDuration);
    }
   
    return (m_state == eAssetUploaded);
}

void ZoneTerrain::update(const GameUpdate &gu)
{
    m_palette->animate(gu.currentTime);
}

static bool regionLessThan(const RegionActor *a, const RegionActor *b)
{
    return a->regionID() < b->regionID();
}

void ZoneTerrain::draw(RenderProgram *prog)
{
    RenderContext *renderCtx = m_zone->game()->renderContext();
    if(!renderCtx->isValid())
        return;
    
    //AutoTimerStat s2(m_zoneStatGPU, renderCtx, "Zone GPU (ms)", FrameStat::GPUTime);
    
    // Import material groups from the visible parts.
    m_zoneBuffer->matGroups.clear();
    qSort(m_visibleRegions.begin(), m_visibleRegions.end(), regionLessThan);
    uint32_t visibleRegions = m_visibleRegions.size();
    for(uint32_t i = 0; i < visibleRegions; i++)
    {
        RegionActor *staticActor = m_visibleRegions[i];
        if(staticActor)
            m_zoneBuffer->addMaterialGroups(staticActor->mesh()->data());
    }
    
    // Draw the visible parts as one big mesh.
    MaterialMap *materialMap = m_palette->map();
    m_batch->setBuffer(m_zoneBuffer);
    m_batch->setMaterials(m_palette->array());
    m_batch->setMaterialMaps(&materialMap);
    prog->drawMesh(*m_batch);
}
