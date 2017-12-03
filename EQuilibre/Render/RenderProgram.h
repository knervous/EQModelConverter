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

#ifndef EQUILIBRE_SHADER_PROGRAM_GL2_H
#define EQUILIBRE_SHADER_PROGRAM_GL2_H

#include <vector>
#include <QString>
#include <QVarLengthArray>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/RenderContext.h"

const int MAX_TRANSFORMS = 256;
const int MAX_MATERIAL_SLOTS = 256;
const int MAX_LIGHTS = 8;

const int A_POSITION = 0;
const int A_NORMAL = 1;
const int A_TEX_COORDS = 2;
const int A_COLOR = 3;
const int A_BONE_INDEX = 4;
const int A_MODEL_VIEW_0 = 5;
const int A_MAX = A_MODEL_VIEW_0;

const int U_MODELVIEW_MATRIX = 0;
const int U_PROJECTION_MATRIX = 1;
const int U_AMBIENT_LIGHT = 2;
const int U_MAT_HAS_TEXTURE = 3;
const int U_MAT_TEXTURE = 4;
const int U_MAT_SLOT_MAP_ENABLED = 5;
const int U_MAT_SLOT_MAP = 6;
const int U_LIGHTING_MODE = 7;
const int U_FOG_START = 8;
const int U_FOG_END = 9;
const int U_FOG_DENSITY = 10;
const int U_FOG_COLOR = 11;
const int U_ANIM_TEX = 12;
const int U_ANIM_TEX_SIZE = 13;
const int U_ANIM_ID = 14;
const int U_ANIM_FRAME = 15;
const int U_MAX = U_ANIM_FRAME;

struct  ShaderSymbolInfo
{
    uint32_t ID;
    const char *Name;
};

class AnimationArray;
class BoneTransform;
class Material;
class MaterialArray;
class MaterialMap;
class RenderBatch;

class  RenderProgram
{
public:
    RenderProgram(RenderContext *renderCtx);
    virtual ~RenderProgram();

    bool loaded() const;
    bool current() const;
    uint32_t program() const;
    int drawCalls() const;
    int textureBinds() const;
    void resetFrameStats();

    bool load(QString vertexFile, QString fragmentFile);

    virtual bool init();
    
    /**
     * @brief Prepare the GPU for drawing one or more mesh with the same geometry.
     * For example, send the geometry to the GPU if it isn't there already.
     */
    virtual void beginBatch(RenderBatch &batch);
    
    /**
     * @brief Draw several instances of a mesh whose geometry was passed to
     * @ref beginBatch, each with a different model-view matrix.
     */
    virtual void drawInstances(RenderBatch &batch, uint32_t instances);
    /**
     * @brief Clean up the resources used by @ref beginBatch and allow it to be
     * called again.
     */
    virtual void endBatch(RenderBatch &batch);
    
    /**
     * @brief Draw a single instance of a mesh. This is equivalent to calling
     * @ref beginBatch, @ref drawInstances and @ref endBatch.
     */
    virtual void drawMesh(RenderBatch &batch);
    
    // debug operations
    void drawBox(const AABox &box);
    void drawFrustum(const Frustum &frustum);
    
    enum LightingMode
    {
        NoLighting = 0,
        BakedLighting = 1,
        DebugVertexColor = 2,
        DebugTextureFactor = 3,
        DebugDiffuse = 4
    };

    void setModelViewMatrix(const matrix4 &modelView);
    void setProjectionMatrix(const matrix4 &projection);
    void setAmbientLight(vec4 lightColor);
    void setLightingMode(LightingMode newMode);
    void setFogParams(const FogParams &fogParams);

protected:
    bool compileProgram(QString vertexFile, QString fragmentFile);
    static uint32_t loadShader(QString path, uint32_t type);
    void enableVertexAttribute(int attr, int index = 0);
    void disableVertexAttribute(int attr, int index = 0);
    void uploadVertexAttributes(const MeshBuffer *meshBuf);
    void beginApplyMaterial(texture_t tex, bool isOpaque);
    void endApplyMaterial(texture_t tex, bool isOpaque);
    uint32_t importMaterialGroups(RenderBatch &batch);
    void drawMaterialGroup(RenderBatch &batch, const MaterialGroup &mg);
    void bindColorBuffer(RenderBatch &batch, int instanceID, bool &enabledColor);
    void setMaterialMap(RenderBatch &batch, MaterialMap *materialMap);
    virtual void beginSkinMesh(RenderBatch &batch);
    virtual void skinMeshInstance(RenderBatch &batch, unsigned instanceID);
    virtual void endSkinMesh(RenderBatch &batch);
    void createCube();
    void uploadCube();

    RenderContext *m_renderCtx;
    uint32_t m_vertexShader;
    uint32_t m_fragmentShader;
    uint32_t m_program;
    int m_attr[A_MAX+1];
    int m_uniform[U_MAX+1];
    std::vector<BoneTransform> m_bones;
    int m_drawCalls;
    int m_textureBinds;
    bool m_projectionSent;
    bool m_blendingEnabled;
    bool m_currentMatNeedsBlending;
    MeshBuffer *m_cube;
    MaterialArray *m_cubeMats;
};

class TextureSkinningProgram : public RenderProgram
{
public:
    TextureSkinningProgram(RenderContext *renderCtx);
    virtual ~TextureSkinningProgram();
    virtual bool init();
    virtual void beginSkinMesh(RenderBatch &batch);
    virtual void skinMeshInstance(RenderBatch &batch, unsigned instanceID);
    virtual void endSkinMesh(RenderBatch &batch);
};

/**
  * @brief Holds the resources used to draw a batch of meshes.
 */
class  RenderBatch
{
public:
    RenderBatch();
    RenderBatch(const MeshBuffer *meshBuf, MaterialArray *materials);
    void clear();
    
    // Batch state.
    void setBuffer(const MeshBuffer *meshBuf);
    void setMaterials(MaterialArray *materials);
    void setMaterialTexture(texture_t materialTex);
    void setAnimations(AnimationArray *animArray);
    void setAnimationTexutre(texture_t animTex);
    
    // Per-instance state.
    void setModelViewMatrices(const matrix4 *mvMatrices);
    void setColorSegments(const BufferSegment *colorSegments);
    void setMaterialMaps(MaterialMap **materialMaps);
    void setAnimationIDs(const uint32_t *animIDs);
    void setAnimationFrames(const float *animFrames);
    
    // Shared instance state.
    void setSharedMaterialMap(MaterialMap *materialMap);
    
private:
    friend class RenderProgram;
    friend class TextureSkinningProgram;
    
    // Batch state.
    const MeshBuffer *meshBuf;
    MaterialArray *materials;
    AnimationArray *animArray;
    texture_t animTex;
    
    // Instance state.
    const matrix4 *mvMatrices;
    const BufferSegment *colorSegments;
    MaterialMap **materialMaps;
    MaterialMap *sharedMaterialMap;
    const uint32_t *animIDs;
    const float *animFrames;
    
    // Internal state.
    bool pending;
    QVarLengthArray<MaterialGroup, 32> groups;
    QVarLengthArray<MaterialGroup, 4> mergedGroups;
    QVarLengthArray<Material *, 32> groupMats;
    texture_t sharedTexture;
    bool allOpaque;
};

#endif
