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

#ifndef EQUILIBRE_GAME_ZONE_TERRAIN_H
#define EQUILIBRE_GAME_ZONE_TERRAIN_H

#include <vector>
//#include "Newton.h"
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/Geometry.h"
#include "EQuilibre/Game/GamePacks.h"

class Game;
struct GameUpdate;
class Zone;
class RegionActor;
class MeshBuffer;
struct RegionTreeNode;
class RegionTreeFragment;
class RenderBatch;
class RenderProgram;
class FrameStat;
class WLDMaterialPalette;
class PFSArchive;
class WLDData;

/*!
  \brief Holds the resources needed to render a zone's terrain.
  */
class  ZoneTerrain
{
public:
    ZoneTerrain(Zone *zone);
    virtual ~ZoneTerrain();
    
    AssetLoadState state() const;
    const AABox & bounds() const;
    uint32_t currentRegionID() const;
    void setCurrentRegionID(uint32_t newID);
    const std::vector<RegionActor *> & visibleRegions() const;

    bool load(PFSArchive *archive, WLDData *wld);
    void update(const GameUpdate &gu);
    bool upload();
    void draw(RenderProgram *prog);
    void unload();
    void resetVisible();
    void showAllRegions(const Frustum &frustum);
    void showNearbyRegions(const Frustum &frustum);
    void showCurrentRegion(const Frustum &frustum);
    uint32_t findRegions(Sphere sphere, uint32_t *regions, uint32_t maxRegions);
    RegionActor * findRegionActor(const vec3 &pos);
    uint32_t findRegionID(const vec3 &pos) const;
    RegionActor * regionActor(uint32_t regionID) const;

private:
    uint32_t findRegion(const vec3 &pos, const RegionTreeNode *nodes, uint32_t nodeIdx) const;
    void findRegions(const Sphere &sphere, const RegionTreeNode *nodes, uint32_t nodeIdx,
                     uint32_t *regions, uint32_t maxRegions, uint32_t &found);

    uint32_t m_regionCount;
    uint32_t m_currentRegion;
    Zone *m_zone;
    AssetLoadState m_state;
    std::vector<RegionActor *> m_regionActors;
    std::vector<RegionActor *> m_visibleRegions;
    RegionTreeFragment *m_regionTree;
    fence_t m_uploadFence;
    double m_uploadStart;
    MeshBuffer *m_zoneBuffer;
    WLDMaterialPalette *m_palette;
    RenderBatch *m_batch;
    AABox m_zoneBounds;
    FrameStat *m_zoneStat;
    FrameStat *m_zoneStatGPU;
};

#endif
