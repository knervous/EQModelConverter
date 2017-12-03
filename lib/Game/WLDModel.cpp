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

#include <math.h>
#include <QImage>
#include <QRegExp>
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Core/Fragments.h"
#include "EQuilibre/Core/Skeleton.h"
#include "EQuilibre/Core/PFSArchive.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/Vertex.h"

using namespace std;

CharacterModel::CharacterModel(WLDMesh *mainMesh)
{
    Q_ASSERT(mainMesh != NULL);
    m_buffer = 0;
    m_state = eAssetLoaded;
    m_uploadFence = NULL;
    m_mainMesh = mainMesh;
    m_skel = 0;
    m_animArray = new AnimationArray();
    m_animBuffer = 0;
    m_animTexture = 0;
    m_race = eRaceInvalid;
    m_gender = eGenderMale;
    addPart(mainMesh, 0);
}

CharacterModel::~CharacterModel()
{
    for(unsigned i = 0; i < m_meshes.size(); i++)
        delete m_meshes[i];
    clear(NULL);
    delete m_animArray;
}

RaceID CharacterModel::race() const
{
    return m_race;
}

void CharacterModel::setRace(RaceID newRace)
{
    m_race = newRace;
}

GenderID CharacterModel::gender() const
{
    return m_gender;
}

void CharacterModel::setGender(GenderID newGender)
{
    m_gender = newGender;
}

QString CharacterModel::animBase() const
{
    return m_animBase;
}

void CharacterModel::setAnimBase(QString newBase)
{
    m_animBase = newBase;
}

WLDMesh * CharacterModel::mainMesh() const
{
    return m_mainMesh;
}

const std::vector<WLDMesh *> & CharacterModel::meshes() const
{
    return m_meshes;
}

MeshBuffer * CharacterModel::buffer() const
{
    return m_buffer;
}

void CharacterModel::setBuffer(MeshBuffer *newBuffer)
{
    m_buffer = newBuffer;
}

Skeleton * CharacterModel::skeleton() const
{
    return m_skel;
}

void CharacterModel::setSkeleton(Skeleton *skeleton)
{
    m_skel = skeleton;
}

QList<MeshFragment *> CharacterModel::listMeshes(ActorDefFragment *def)
{
    QList<MeshFragment *> meshes;
    if(!def)
        return meshes;
    foreach(WLDFragment *modelFrag, def->m_models)
    {
        MeshFragment *meshFrag = modelFrag->cast<MeshFragment>();
        if(meshFrag)
            meshes.append(meshFrag);
        HierSpriteFragment *skelFrag = modelFrag->cast<HierSpriteFragment>();
        if(skelFrag)
        {
            foreach(MeshFragment *meshFrag2, skelFrag->m_def->m_meshes)
            {
                if(meshFrag2)
                   meshes.append(meshFrag2);
            }
            const SkeletonTree &tree = skelFrag->m_def->m_tree;
            foreach(SkeletonNode node, tree)
            {
                if(node.mesh)
                    meshes.append(node.mesh);
            }
        }
    }
    return meshes;
}

AnimationArray * CharacterModel::animations() const
{
    return m_animArray;
}

texture_t CharacterModel::animationTexture() const
{
    return m_animTexture;
}

uint32_t CharacterModel::skinCount() const
{
    return m_skinMasks.size();
}

uint32_t CharacterModel::getSkinMask(uint32_t skinID, bool createSkin)
{
    // Get the skin's mask if it exists.
    uint32_t newMask = (m_skinMasks.size() > 0) ? m_skinMasks[0] : 0;
    if(skinID >= m_skinMasks.size())
    {
        if(createSkin)
        {
            m_skinMasks.resize(skinID + 1, newMask);
        }
        else
        {
            return newMask;
        }
    }
    return m_skinMasks[skinID];
}

bool CharacterModel::isPartUsed(uint32_t skinMask, uint32_t partID) const
{
    return (skinMask & (1 << partID));
}

void CharacterModel::setPartUsed(uint32_t &skinMask, uint32_t partID, bool used) const
{
    if(used)
        skinMask |= (1 << partID);
    else
        skinMask &= ~(1 << partID);
}

void CharacterModel::addPart(MeshDefFragment *meshDef, uint32_t skinID)
{
    if(!meshDef)
        return;
    WLDMesh *meshPart = new WLDMesh(meshDef, m_meshes.size());
    addPart(meshPart, skinID);
}

void CharacterModel::addPart(WLDMesh *mesh, uint32_t skinID)
{
    if(!mesh)
        return;
    m_meshes.push_back(mesh);
    
    // Mark the part used by the skin.
    uint32_t skinMask = getSkinMask(skinID, true);
    setPartUsed(skinMask, mesh->partID(), true);
    m_skinMasks[skinID] = skinMask;
}

void CharacterModel::replacePart(MeshDefFragment *meshDef, uint32_t skinID, uint32_t oldPartID)
{
    if(!meshDef || (oldPartID >= m_meshes.size()))
        return;
    addPart(meshDef, skinID);
    uint32_t skinMask = getSkinMask(skinID, true);
    setPartUsed(skinMask, oldPartID, false);
    m_skinMasks[skinID] = skinMask;
}

void CharacterModel::clear(RenderContext *renderCtx)
{
    MaterialArray *matArray = mainMesh()->palette()->array();
    if(renderCtx)
    {
        if(m_buffer)
        {
            m_buffer->clear(renderCtx);
        }
        if(matArray)
        {
            matArray->clear(renderCtx);
        }
        if(m_animTexture)
        {
            renderCtx->freeTexture(m_animTexture);
            m_animTexture = 0;
        }
        if(m_animBuffer)
        {
            renderCtx->freeBuffers(&m_animBuffer, 1);
            m_animBuffer = 0;
        }
    }
    delete m_buffer;
    m_buffer = NULL;
    
    // CharacterModel does not own the skeleton.
    m_skel = NULL;
    m_animArray->clear();
}

bool CharacterModel::upload(RenderContext *renderCtx)
{
    if(!renderCtx || !renderCtx->isValid())
    {
        return false;
    }
    
    MaterialArray *materials = m_mainMesh->palette()->createArray(); // XXX needed?
    if(m_state == eAssetLoaded)
    {
        // Start uploading the textures and update the material subtextures.
        materials->uploadArray(renderCtx);
        m_state = eAssetUploadingTextures;
        
        // Import mesh geometry.
        m_buffer = new MeshBuffer();
        foreach(WLDMesh *mesh, m_meshes)
        {
            mesh->importFrom(m_buffer);
            mesh->data()->updateTexCoords(materials, true);
        }
    
        // Create the GPU buffers.
        m_buffer->upload(renderCtx);
    
        // Free the memory used for indices.
        // We need to keep the vertices around for software skinning.
        m_buffer->clearIndices();
        
        // Create a texture from the animation data.
        m_animBuffer = renderCtx->createBuffer(m_animArray->data(),
                                               m_animArray->dataSize());
        m_animTexture = renderCtx->createTextureFromBuffer(m_animBuffer);
        
        // Create a fence to know when uploading is done.
        m_uploadFence = renderCtx->createFence();
    }
    
    if((m_state == eAssetUploadingTextures) &&
       (materials->checkUpload(renderCtx) == eUploadFinished) &&
       renderCtx->isFenceSignaled(m_uploadFence))
    {
        m_state = eAssetUploaded;
        renderCtx->deleteFence(m_uploadFence);
        m_uploadFence = NULL;
    }
   
    return (m_state == eAssetUploaded);
}

////////////////////////////////////////////////////////////////////////////////

WLDMesh::WLDMesh(MeshDefFragment *meshDef, uint32_t partID)
{
    Q_ASSERT(meshDef != NULL);
    m_partID = partID;
    m_meshDef = meshDef;
    m_data = NULL;
    m_palette = NULL;
    m_boundsAA.low = meshDef->m_boundsAA.low + meshDef->m_center;
    m_boundsAA.high = meshDef->m_boundsAA.high + meshDef->m_center;
    meshDef->setHandled(true);
}

WLDMesh::~WLDMesh()
{
    delete m_palette;
}

MeshData * WLDMesh::data() const
{
    return m_data;
}

void WLDMesh::setData(MeshData *mesh)
{
    m_data = mesh;
}

MeshDefFragment * WLDMesh::def() const
{
    return m_meshDef;
}

WLDMaterialPalette * WLDMesh::palette() const
{
    return m_palette;
}

uint32_t WLDMesh::partID() const
{
    return m_partID;
}

const AABox & WLDMesh::boundsAA() const
{
    return m_boundsAA;
}

WLDMaterialPalette * WLDMesh::importPalette(PFSArchive *archive)
{
    m_palette = new WLDMaterialPalette(archive);
    m_palette->setDef(m_meshDef->m_palette);
    m_palette->createSlots();
    return m_palette;
}

MeshData * WLDMesh::importFrom(MeshBuffer *meshBuf, uint32_t paletteOffset)
{
    MeshData *meshData = importMaterialGroups(meshBuf, paletteOffset);
    importVertexData(meshBuf, meshData->vertexSegment);
    importIndexData(meshBuf, meshData->indexSegment, meshData->vertexSegment,
                    0, (uint32_t)m_meshDef->m_indices.count());
    return meshData;
}

void WLDMesh::importVertexData(MeshBuffer *buffer, BufferSegment &dataLoc)
{
    // Update the location of the mesh in the buffer.
    QVector<Vertex> &vertices(buffer->vertices);
    uint32_t vertexCount = (uint32_t)m_meshDef->m_vertices.count();
    uint32_t vertexIndex = vertices.count();
    dataLoc.offset = vertexIndex;
    dataLoc.count = vertexCount;
    dataLoc.elementSize = sizeof(Vertex);
    
    // Load vertices, texCoords, normals, faces.
    bool hasColors = m_meshDef->m_colors.count() > 0;
    uint32_t defaultColorABGR = 0xbfffffff; // A=0.75, B=1, G=1, R=1
    for(uint32_t i = 0; i < vertexCount; i++)
    {
        Vertex v;
        v.position = m_meshDef->m_vertices.value(i) + m_meshDef->m_center;
        v.normal = m_meshDef->m_normals.value(i);
        v.texCoords = vec3(m_meshDef->m_texCoords.value(i), 0.0f);
        v.color = hasColors ? m_meshDef->m_colors.value(i) : defaultColorABGR;
        v.bone = 0;
        vertices.append(v);
    }
    
    // Load bone indices.
    foreach(vec2us g, m_meshDef->m_vertexPieces)
    {
        uint16_t count = g.first, pieceID = g.second;
        for(uint32_t i = 0; i < count; i++, vertexIndex++)
            vertices[vertexIndex].bone = pieceID;
    }
}

void WLDMesh::importIndexData(MeshBuffer *buffer, BufferSegment &indexLoc,
                                   const BufferSegment &dataLoc, uint32_t offset, uint32_t count)
{
    QVector<uint32_t> &indices = buffer->indices;
    indexLoc.offset = indices.count();
    indexLoc.count = count;
    indexLoc.elementSize = sizeof(uint32_t);
    for(uint32_t i = 0; i < count; i++)
        indices.push_back(m_meshDef->m_indices[i + offset] + dataLoc.offset);
}

MeshData * WLDMesh::importMaterialGroups(MeshBuffer *buffer, uint32_t paletteOffset)
{
    // Load material groups.
    uint32_t meshOffset = 0;
    m_data = buffer->createMesh(m_meshDef->m_polygonsByTex.count());
    for(int i = 0; i < m_meshDef->m_polygonsByTex.count(); i++)
    {
        vec2us g = m_meshDef->m_polygonsByTex[i];
        uint32_t vertexCount = g.first * 3;
        MaterialGroup &mg(m_data->matGroups[i]);
        mg.id = m_partID;
        mg.offset = meshOffset;
        mg.count = vertexCount;
        mg.matID = paletteOffset + g.second;
        meshOffset += vertexCount;
    }
    return m_data;
}

static bool materialGroupLessThan(const MaterialGroup &a, const MaterialGroup &b)
{
    return a.matID < b.matID;
}

MeshBuffer * WLDMesh::combine(const QVector<WLDMesh *> &meshes)
{
    // import each part (vertices and material groups) into a single vertex group
    MeshBuffer *meshBuf = new MeshBuffer();
    foreach(WLDMesh *mesh, meshes)
    {
        MeshData *meshData = mesh->importMaterialGroups(meshBuf, 0);
        meshBuf->addMaterialGroups(meshData);
        mesh->importVertexData(meshBuf, meshData->vertexSegment);
    }

    // sort the polygons per material and import indices
    qSort(meshBuf->matGroups.begin(), meshBuf->matGroups.end(), materialGroupLessThan);
    uint32_t indiceOffset = 0;
    for(int i = 0; i < meshBuf->matGroups.count(); i++)
    {
        MaterialGroup &mg(meshBuf->matGroups[i]);
        WLDMesh *mesh = meshes[mg.id];
        mesh->importIndexData(meshBuf, mesh->data()->indexSegment, mesh->data()->vertexSegment,
                              mg.offset, mg.count);
        mg.offset = indiceOffset;
        indiceOffset += mg.count;
    }

    // merge material groups with common material
    QVector<MaterialGroup> newGroups;
    MaterialGroup group;
    group.id = meshBuf->matGroups[0].id;
    group.offset = 0;
    group.count = 0;
    group.matID = meshBuf->matGroups[0].matID;
    for(int i = 0; i < meshBuf->matGroups.count(); i++)
    {
        MaterialGroup &mg(meshBuf->matGroups[i]);
        if(mg.matID != group.matID)
        {
            // new material - output the current group
            newGroups.append(group);
            group.id = mg.id;
            group.offset += group.count;
            group.count = 0;
            group.matID = mg.matID;
        }
        group.count += mg.count;
    }
    newGroups.append(group);
    meshBuf->matGroups = newGroups;
    return meshBuf;
}
