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

#ifndef EQUILIBRE_GAME_WLD_MODEL_H
#define EQUILIBRE_GAME_WLD_MODEL_H

#include <vector>
#include <QList>
#include <QMap>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/Character.h"
#include "EQuilibre/Core/Geometry.h"
#include "EQuilibre/Core/Skeleton.h"
#include "EQuilibre/Game/GamePacks.h"

class MeshFragment;
class MeshDefFragment;
class MaterialDefFragment;
class MaterialPaletteFragment;
class ActorDefFragment;
class AnimationArray;
class Material;
class MaterialArray;
class MaterialMap;
class PFSArchive;
class RenderContext;
class RenderProgram;
class MeshData;
class MeshBuffer;
struct BufferSegment;
class WLDMesh;
class WLDModelSkin;
class WLDMaterialPalette;

/*!
  \brief Describes a model of a character that can be rendered.
  */
class  CharacterModel
{
public:
    CharacterModel(WLDMesh *mainMesh);
    virtual ~CharacterModel();
    
    RaceID race() const;
    void setRace(RaceID newRace);
    
    GenderID gender() const;
    void setGender(GenderID newGender);
    
    /** Name of the model to copy animations from. */
    QString animBase() const;
    void setAnimBase(QString newBase);

    static QList<MeshFragment *> listMeshes(ActorDefFragment *def);
    WLDMesh *mainMesh() const;
    const std::vector<WLDMesh *> & meshes() const;

    MeshBuffer * buffer() const;
    void setBuffer(MeshBuffer *newBuffer);

    Skeleton * skeleton() const;
    void setSkeleton(Skeleton *skeleton);

    AnimationArray * animations() const;
    texture_t animationTexture() const;
    
    uint32_t skinCount() const;
    uint32_t getSkinMask(uint32_t skinID, bool createSkin = false);
    bool isPartUsed(uint32_t skinMask, uint32_t partID) const;
    void setPartUsed(uint32_t &skinMask, uint32_t partID, bool used) const;
    void addPart(MeshDefFragment *meshDef, uint32_t skinID);
    void replacePart(MeshDefFragment *meshDef, uint32_t skinID, uint32_t oldPartID);
    
    void clear(RenderContext *renderCtx);
    bool upload(RenderContext *renderCtx);

private:
    void addPart(WLDMesh *mesh, uint32_t skinID);
    
    friend class WLDModelSkin;
    MeshBuffer *m_buffer;
    AssetLoadState m_state;
    fence_t m_uploadFence;
    Skeleton *m_skel;
    AnimationArray *m_animArray;
    buffer_t m_animBuffer;
    texture_t m_animTexture;
    WLDMesh *m_mainMesh;
    std::vector<WLDMesh *> m_meshes;
    /** List of bitmasks that indicates, for each skin, which meshes are used. */
    std::vector<uint32_t> m_skinMasks;
    RaceID m_race;
    GenderID m_gender;
    QString m_animBase;
};

/*!
  \brief Describes part of a model.
  */
class  WLDMesh
{
public:
    WLDMesh(MeshDefFragment *meshDef, uint32_t partID = 0);
    virtual ~WLDMesh();

    MeshData * data() const;
    void setData(MeshData *data);
    MeshDefFragment *def() const;
    WLDMaterialPalette * palette() const;
    uint32_t partID() const;
    const AABox & boundsAA() const;

    WLDMaterialPalette * importPalette(PFSArchive *archive);
    MeshData * importFrom(MeshBuffer *meshBuf, uint32_t paletteOffset = 0);
    static MeshBuffer *combine(const QVector<WLDMesh *> &meshes);

private:
    void importVertexData(MeshBuffer *buffer, BufferSegment &dataLoc);
    void importIndexData(MeshBuffer *buffer, BufferSegment &indexLoc,
                         const BufferSegment &dataLoc, uint32_t offset, uint32_t count);
    MeshData * importMaterialGroups(MeshBuffer *buffer, uint32_t paletteOffset);
    
    uint32_t m_partID;
    MeshData *m_data;
    MeshDefFragment *m_meshDef;
    WLDMaterialPalette *m_palette;
    AABox m_boundsAA;
};

#endif
