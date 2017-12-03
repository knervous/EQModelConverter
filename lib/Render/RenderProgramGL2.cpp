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

#include <include/glew-1.9.0/include/GL/glew.h>
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Core/Skeleton.h"

static const ShaderSymbolInfo Uniforms[] =
{
    {U_MODELVIEW_MATRIX, "u_modelViewMatrix"},
    {U_PROJECTION_MATRIX, "u_projectionMatrix"},
    {U_AMBIENT_LIGHT, "u_ambientLight"},
    {U_MAT_HAS_TEXTURE, "u_has_texture"},
    {U_MAT_TEXTURE, "u_material_texture"},
    {U_MAT_SLOT_MAP_ENABLED, "u_mapMaterialSlots"},
    {U_MAT_SLOT_MAP, "u_materialSlotMap"},
    {U_LIGHTING_MODE, "u_lightingMode"},
    {U_FOG_START, "u_fogStart"},
    {U_FOG_END, "u_fogEnd"},
    {U_FOG_DENSITY, "u_fogDensity"},
    {U_FOG_COLOR, "u_fogColor"},
    {U_ANIM_TEX, "u_animTex"},
    {U_ANIM_TEX_SIZE, "u_animTexSize"},
    {U_ANIM_ID, "u_animID"},
    {U_ANIM_FRAME, "u_animFrame"},
    {0, NULL}
};

static const ShaderSymbolInfo Attributes[] =
{
    {A_POSITION, "a_position"},
    {A_NORMAL, "a_normal"},
    {A_TEX_COORDS, "a_texCoords"},
    {A_COLOR, "a_color"},
    {A_BONE_INDEX, "a_boneIndex"},
    {A_MODEL_VIEW_0, "a_modelViewMatrix"},
    {0, NULL}
};

RenderProgram::RenderProgram(RenderContext *renderCtx)
{
    m_renderCtx = renderCtx;
    m_program = 0;
    m_vertexShader = 0;
    m_fragmentShader = 0;
    for(int i = 0; i <= A_MAX; i++)
        m_attr[i] = -1;
    for(int i = 0; i <= U_MAX; i++)
        m_uniform[i] = -1;
    m_drawCalls = 0;
    m_textureBinds = 0;
    m_projectionSent = false;
    m_blendingEnabled = m_currentMatNeedsBlending = false;
    m_bones.reserve(MAX_TRANSFORMS);
    m_cube = NULL;
    m_cubeMats = NULL;
    createCube();
}

RenderProgram::~RenderProgram()
{
    delete m_cubeMats;
    delete m_cube;

    if(current())
        glUseProgram(0);
    if(m_vertexShader != 0)
        glDeleteShader(m_vertexShader);
    if(m_fragmentShader != 0)
        glDeleteShader(m_fragmentShader);
    if(m_program != 0)
        glDeleteProgram(m_program);
}

bool RenderProgram::loaded() const
{
    return m_program != 0;
}

bool RenderProgram::load(QString vertexFile, QString fragmentFile)
{
    if(loaded())
        return false;
    else if(!compileProgram(vertexFile, fragmentFile))
        return false;
    else if(!init())
        return false;
    else
        return true;
}

bool RenderProgram::current() const
{
    uint32_t currentProg = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, (int32_t *)&currentProg);
    return (currentProg != 0) && (currentProg == m_program);
}

uint32_t RenderProgram::program() const
{
    return m_program;
}

int RenderProgram::drawCalls() const
{
    return m_drawCalls;
}

int RenderProgram::textureBinds() const
{
    return m_textureBinds;
}

void RenderProgram::resetFrameStats()
{
    m_drawCalls = 0;
    m_textureBinds = 0;
    m_projectionSent = false;
}

bool RenderProgram::init()
{
    return true;
}

bool RenderProgram::compileProgram(QString vertexFile, QString fragmentFile)
{
    uint32_t vertexShader = loadShader(vertexFile, GL_VERTEX_SHADER);
    if(vertexShader == 0)
        return false;
    uint32_t fragmentShader = loadShader(fragmentFile, GL_FRAGMENT_SHADER);
    if(fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return false;
    }
    uint32_t program = glCreateProgram();
    if(program == 0)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(!status)
    {
        GLint log_size;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size)
        {
            GLchar *log = new GLchar[log_size];
            glGetProgramInfoLog(program, log_size, 0, log);
            fprintf(stderr, "Error linking program: %s\n", log);
            delete [] log;
        }
        else
        {
            fprintf(stderr, "Error linking program.\n");
        }
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    m_vertexShader = vertexShader;
    m_fragmentShader = fragmentShader;
    m_program = program;
    
    // Get the location of all uniforms and attributes.
    const ShaderSymbolInfo *uni = Uniforms;
    while(uni->Name)
    {
        m_uniform[uni->ID] = glGetUniformLocation(program, uni->Name);
        uni++;
    }
    const ShaderSymbolInfo *attr = Attributes;
    while(attr->Name)
    {
        m_attr[attr->ID] = glGetAttribLocation(program, attr->Name);
        attr++;
    }
    return true;
}

uint32_t RenderProgram::loadShader(QString path, uint32_t type)
{
    char *code = loadFileData(path.toStdString());
    if(!code)
        return 0;
    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar **)&code, 0);
    freeFileData(code);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        GLint log_size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size)
        {
            GLchar *log = new GLchar[log_size];
            glGetShaderInfoLog(shader, log_size, 0, log);
            fprintf(stderr, "Error compiling shader: %s\n", log);
            delete [] log;
        }
        else
        {
            fprintf(stderr, "Error compiling shader.\n");
        }
        return 0;
    }
    return shader;
}

void RenderProgram::setModelViewMatrix(const matrix4 &modelView)
{
    glUniformMatrix4fv(m_uniform[U_MODELVIEW_MATRIX],
            1, GL_FALSE, (const GLfloat *)modelView.columns());
}

void RenderProgram::setProjectionMatrix(const matrix4 &projection)
{
    glUniformMatrix4fv(m_uniform[U_PROJECTION_MATRIX],
        1, GL_FALSE, (const GLfloat *)projection.columns());
}

void RenderProgram::setMaterialMap(RenderBatch &batch, MaterialMap *materialMap)
{
    uint32_t count = materialMap ? materialMap->count() : 0;
    Q_ASSERT(count <= MAX_MATERIAL_SLOTS);
    if(count > 0)
    {
        float textureMap[MAX_MATERIAL_SLOTS * 3];
        materialMap->fillTextureMap(batch.materials, (vec3 *)textureMap, MAX_MATERIAL_SLOTS);
        glUniform3fv(m_uniform[U_MAT_SLOT_MAP], MAX_MATERIAL_SLOTS, (const GLfloat *)textureMap);
        glUniform1i(m_uniform[U_MAT_SLOT_MAP_ENABLED], 1);
    }
    else
    {
        glUniform1i(m_uniform[U_MAT_SLOT_MAP_ENABLED], 0);
    }
}

void RenderProgram::setAmbientLight(vec4 lightColor)
{
    glUniform4fv(m_uniform[U_AMBIENT_LIGHT], 1, (const GLfloat *)&lightColor);
}

void RenderProgram::setLightingMode(RenderProgram::LightingMode newMode)
{
    glUniform1i(m_uniform[U_LIGHTING_MODE], newMode);
}

void RenderProgram::setFogParams(const FogParams &fogParams)
{
    glUniform1f(m_uniform[U_FOG_START], fogParams.start);
    glUniform1f(m_uniform[U_FOG_END], fogParams.end);
    glUniform1f(m_uniform[U_FOG_DENSITY], fogParams.density);
    glUniform4fv(m_uniform[U_FOG_COLOR], 1, (const GLfloat *)&fogParams.color);
}

void RenderProgram::beginApplyMaterial(texture_t tex, bool isOpaque)
{
    GLuint target = GL_TEXTURE_2D_ARRAY;
    if(tex != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(target, tex);
        glUniform1i(m_uniform[U_MAT_TEXTURE], 0);
        glUniform1i(m_uniform[U_MAT_HAS_TEXTURE], 1);
        m_textureBinds++;
    }
    else
    {
        glUniform1i(m_uniform[U_MAT_HAS_TEXTURE], 0);
    }
    m_currentMatNeedsBlending = !isOpaque;
}

void RenderProgram::endApplyMaterial(texture_t tex, bool isOpaque)
{
    GLuint target = GL_TEXTURE_2D_ARRAY;
    if(tex != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(target, 0);
    }
    m_currentMatNeedsBlending = false;
}

void RenderProgram::enableVertexAttribute(int attr, int index)
{
    if(m_attr[attr] >= 0)
        glEnableVertexAttribArray(m_attr[attr] + index);
}

void RenderProgram::disableVertexAttribute(int attr, int index)
{
    if(m_attr[attr] >= 0)
        glDisableVertexAttribArray(m_attr[attr] + index);
}

void RenderProgram::uploadVertexAttributes(const MeshBuffer *meshBuf)
{
    glBindBuffer(GL_ARRAY_BUFFER, meshBuf->vertexBuffer);
    const uint8_t *posPointer = 0;
    const uint8_t *normalPointer = posPointer + sizeof(vec3);
    const uint8_t *texCoordsPointer = normalPointer + sizeof(vec3);
    const uint8_t *colorPointer = texCoordsPointer + sizeof(vec3);
    const uint8_t *bonePointer = colorPointer + sizeof(uint32_t);
    glVertexAttribPointer(m_attr[A_POSITION], 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), posPointer);
    if(m_attr[A_NORMAL] >= 0)
        glVertexAttribPointer(m_attr[A_NORMAL], 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), normalPointer);
    if(m_attr[A_TEX_COORDS] >= 0)
        glVertexAttribPointer(m_attr[A_TEX_COORDS], 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), texCoordsPointer);
    if(m_attr[A_COLOR] >= 0)
        glVertexAttribPointer(m_attr[A_COLOR], 4, GL_UNSIGNED_BYTE, GL_TRUE,
            sizeof(Vertex), colorPointer);
    if(m_attr[A_BONE_INDEX] >= 0)
        glVertexAttribPointer(m_attr[A_BONE_INDEX], 1, GL_INT, GL_FALSE,
            sizeof(Vertex), bonePointer);
}

void RenderProgram::beginBatch(RenderBatch &batch)
{
    const MeshBuffer *meshBuf = batch.meshBuf;
    if(batch.pending || !meshBuf || !meshBuf->vertexBuffer || !batch.materials)
        return;
    batch.pending = true;
    if(!m_projectionSent)
    {
        setProjectionMatrix(m_renderCtx->matrix(RenderContext::Projection));
        m_projectionSent = true;
    }
    enableVertexAttribute(A_POSITION);
    enableVertexAttribute(A_NORMAL);
    enableVertexAttribute(A_TEX_COORDS);
    if(batch.animArray)
        enableVertexAttribute(A_BONE_INDEX);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuf->indexBuffer);
    if(batch.animArray)
        beginSkinMesh(batch);
    uploadVertexAttributes(meshBuf);
    setMaterialMap(batch, NULL);
    importMaterialGroups(batch);
    
    // Merge consecutive material groups if they all use the same texture.
    if(batch.sharedTexture)
    {
        MaterialGroup merged;
        unsigned groupCount = batch.groups.size();
        unsigned j = 0;
        do
        {
            merged.offset = batch.groups[j].offset;
            merged.count = batch.groups[j].count;
            j++;
            for(; j < groupCount; j++)
            {
                uint32_t mergedEnd = merged.offset + merged.count;
                if(batch.groups[j].offset != mergedEnd)
                {
                    break;
                }
                merged.count += batch.groups[j].count;
            }
            batch.mergedGroups.append(merged);
        } while(j < groupCount);
    }
}

uint32_t RenderProgram::importMaterialGroups(RenderBatch &batch)
{
    // If the user specified a texture to use for all material groups,
    // we can just copy the whole list and not care about materials.
    const MeshBuffer *meshBuf = batch.meshBuf;
    unsigned groupCount = meshBuf->matGroups.count();
    if(batch.sharedTexture)
    {
        unsigned groupSize = groupCount * sizeof(MaterialGroup);
        memcpy(batch.groups.data(), meshBuf->matGroups.data(), groupSize);
        return groupCount;
    }
    
    // Get a Material pointer for each material group.
    for(unsigned i = 0; i < meshBuf->matGroups.count(); i++)
    {
        uint32_t matID = meshBuf->matGroups[i].matID;
        Material *mat = batch.materials->material(matID);
        // Skip meshes that don't have a material.
        if(!mat)
        {
            groupCount--;
            continue;
        }
        batch.groups.append(meshBuf->matGroups[i]);
        batch.groupMats.append(mat);
    }

    // Check whether all materials use the same texture (array) or not.
    batch.allOpaque = true;
    batch.sharedTexture = 0;
    if(groupCount > 0)
    {
        Material *firstMat = batch.groupMats[0];
        batch.allOpaque = firstMat->isOpaque();
        batch.sharedTexture = firstMat->texture();
        for(int i = 1; i < groupCount; i++)
        {
            Material *otherMat = batch.groupMats[i];
            batch.allOpaque &= otherMat->isOpaque();
            if(batch.sharedTexture != otherMat->texture())
            {
                batch.sharedTexture = 0;
                break;
            }
        }
    }
    return groupCount;
}

void RenderProgram::drawMesh(RenderBatch &batch)
{
    beginBatch(batch);
    if(!batch.mvMatrices)
        batch.mvMatrices = &m_renderCtx->matrix(RenderContext::ModelView);
    drawInstances(batch, 1);
    endBatch(batch);
}

void RenderProgram::drawInstances(RenderBatch &batch, uint32_t instances)
{
    // No material group - nothing is drawn.
    unsigned groupCount = batch.groups.size();
    unsigned mergedCount = batch.mergedGroups.size();
    if(!batch.pending || (groupCount == 0))
        return;

    bool enabledColor = false;
    if(batch.sharedTexture)
    {
        // If all material groups use the same texture we can render them together.
        if(batch.sharedMaterialMap && !batch.materialMaps)
            setMaterialMap(batch, batch.sharedMaterialMap);
        beginApplyMaterial(batch.sharedTexture, batch.allOpaque);
        for(uint32_t i = 0; i < instances; i++)
        {
            // Set per-instance state.
            setModelViewMatrix(batch.mvMatrices[i]);
            bindColorBuffer(batch, i, enabledColor);
            if(batch.animArray)
                skinMeshInstance(batch, i);
            if(batch.materialMaps && !batch.sharedMaterialMap)
                setMaterialMap(batch, batch.materialMaps[i]);
            
            // Draw material groups.
            for(uint32_t j = 0; j < mergedCount; j++)
                drawMaterialGroup(batch, batch.mergedGroups[j]);
        }
        endApplyMaterial(batch.sharedTexture, batch.allOpaque);
    }
    else
    {
        // Otherwise we have to change the texture for every material group.
        for(int i = 0; i < groupCount; i++)
        {
            Material *mat = batch.groupMats[i];
            beginApplyMaterial(mat->texture(), mat->isOpaque());
            for(uint32_t j = 0; j < instances; j++)
            {
                setModelViewMatrix(batch.mvMatrices[j]);
                bindColorBuffer(batch, i, enabledColor);
                // Material maps are only supported for single-texture meshes.
                if(batch.animArray)
                    skinMeshInstance(batch, i);
                drawMaterialGroup(batch, batch.groups[i]);
            }
            endApplyMaterial(mat->texture(), mat->isOpaque());
        }
    }
    
    if(enabledColor)
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        disableVertexAttribute(A_COLOR);
    }
}

void RenderProgram::bindColorBuffer(RenderBatch &batch, int instanceID, bool &enabledColor)
{
    // Make sure the attribute is actually used by the shader.
    if(m_attr[A_COLOR] < 0)
        return;
    
    const MeshBuffer *meshBuf = batch.meshBuf;
    if(batch.colorSegments)
    {
        // Use the actor's per-instance color buffer.
        BufferSegment colorSegment = batch.colorSegments[instanceID];
        if(colorSegment.count > 0)
        {
            if(!enabledColor)
            {
                enableVertexAttribute(A_COLOR);
                enabledColor = true;
            }
            glBindBuffer(GL_ARRAY_BUFFER, meshBuf->colorBuffer);
            glVertexAttribPointer(m_attr[A_COLOR], 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                  0, (GLvoid *)colorSegment.address());
        }
        else if(enabledColor)
        {
            // No color information for this actor, do not reuse the previous actor's.
            disableVertexAttribute(A_COLOR);
            enabledColor = false;
        }
    }
    else
    {
        // Use the color inside the mesh's vertex buffer.
        if(!enabledColor)
        {
            const uint8_t *colorPtr = (uint8_t *)offsetof(Vertex, color);
            enableVertexAttribute(A_COLOR);
            glBindBuffer(GL_ARRAY_BUFFER, meshBuf->vertexBuffer);
            glVertexAttribPointer(m_attr[A_COLOR], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), colorPtr);
            enabledColor = true;
        }
    }
}

void RenderProgram::drawMaterialGroup(RenderBatch &batch, const MaterialGroup &mg)
{
    // Enable or disable blending based on the current material.
    if(m_currentMatNeedsBlending && !m_blendingEnabled)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_blendingEnabled = true;
    }
    else if(!m_currentMatNeedsBlending && m_blendingEnabled)
    {
        glDisable(GL_BLEND);
        m_blendingEnabled = false;
    }
    
    const GLuint mode = GL_TRIANGLES;
    if(batch.meshBuf->indexBuffer)
        glDrawElements(mode, mg.count, GL_UNSIGNED_INT, (uint32_t *)0 + mg.offset);
    else
        glDrawArrays(mode, mg.offset, mg.count);
    m_drawCalls++;
}

void RenderProgram::endBatch(RenderBatch &batch)
{
    if(!batch.pending)
        return;
    if(batch.animArray)
        endSkinMesh(batch);
    //XXX No unbind / do unbind at the end of the frame.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    for(int i = 0; i < 4; i++)
        disableVertexAttribute(A_MODEL_VIEW_0, i);
    disableVertexAttribute(A_BONE_INDEX);
    disableVertexAttribute(A_POSITION);
    disableVertexAttribute(A_NORMAL);
    disableVertexAttribute(A_TEX_COORDS);
    disableVertexAttribute(A_COLOR);
    batch.clear();
}

void RenderProgram::beginSkinMesh(RenderBatch &batch)
{
}

void RenderProgram::skinMeshInstance(RenderBatch &batch, unsigned instanceID)
{
    const MeshBuffer *meshBuf = batch.meshBuf;
    if(!batch.animArray)
        return;
    
    // Get the current transformation for each bone.
    uint32_t animID = batch.animIDs ? batch.animIDs[instanceID] : 0;
    float animFrame = batch.animFrames ? batch.animFrames[instanceID] : 0.0f;
    m_bones.resize(batch.animArray->maxTracks());
    batch.animArray->transformationsAtFrame(animID, animFrame, m_bones);
    
    glBindBuffer(GL_ARRAY_BUFFER, meshBuf->vertexBuffer);
    // Discard the previous data.
    glBufferData(GL_ARRAY_BUFFER, meshBuf->vertexBufferSize, NULL, GL_STREAM_DRAW);
    // Map the VBO in memory and copy skinned vertices directly to it.
    void *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if(!buffer)
        return;
    const Vertex *src = meshBuf->vertices.constData();
    Vertex *dst = (Vertex *)buffer;
    for(uint32_t i = 0; i < meshBuf->vertices.count(); i++, src++, dst++)
    {
        BoneTransform transform = BoneTransform::identity();
        if(src->bone < m_bones.size())
        {
            transform = m_bones[src->bone];
        }
        dst->position = transform.map(src->position);
        dst->normal = src->normal;
        dst->bone = src->bone;
        dst->texCoords = src->texCoords;
        dst->color = src->color;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderProgram::endSkinMesh(RenderBatch &batch)
{
    // Restore the old mesh data that was overwritten by beginSkinMesh.
    const MeshBuffer *meshBuf = batch.meshBuf;
    glBindBuffer(GL_ARRAY_BUFFER, meshBuf->vertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, meshBuf->vertexBufferSize, meshBuf->vertices.constData());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static const vec3 cubeVertices[] =
{
    vec3(-0.5, -0.5,  0.5), vec3(  0.5, -0.5,  0.5),
    vec3( 0.5,  0.5,  0.5), vec3( -0.5,  0.5,  0.5),
    vec3(-0.5, -0.5, -0.5), vec3(  0.5, -0.5, -0.5),
    vec3( 0.5,  0.5, -0.5), vec3( -0.5,  0.5, -0.5)
};

static void fromEightCorners(MeshBuffer *meshBuffer, const vec3 *corners)
{
    static const uint8_t faces_indices[] =
    {
        0, 1, 3,   3, 1, 2,   2, 1, 6,   6, 1, 5,
        0, 4, 1,   1, 4, 5,   3, 2, 7,   7, 2, 6,
        4, 7, 5,   5, 7, 6,   0, 3, 4,   4, 3, 7
    };
    
    Vertex *v = meshBuffer->vertices.data();
    for(uint32_t i = 0; i < 12; i++)
    {
        uint8_t idx1 = faces_indices[(i * 3) + 0];
        uint8_t idx2 = faces_indices[(i * 3) + 1];
        uint8_t idx3 = faces_indices[(i * 3) + 2];
        Vertex &v1 = v[(i * 3) + 0];
        Vertex &v2 = v[(i * 3) + 1];
        Vertex &v3 = v[(i * 3) + 2];
        v1.position = corners[idx1];
        v2.position = corners[idx2];
        v3.position = corners[idx3];
        vec3 u = (v2.position - v1.position), v = (v3.position - v1.position);
        v1.normal = v2.normal = v3.normal = vec3::cross(u, v).normalized();
        v1.texCoords = v2.texCoords = v3.texCoords = vec3(0.0, 0.0, 1.0);
        v1.color = v2.color = v3.color = 0xff000000;
    }
}

void RenderProgram::createCube()
{   
    MaterialGroup mg;
    mg.count = 36;
    mg.offset = 0;
    mg.id = 0;
    mg.matID = 1;
    
    m_cube = new MeshBuffer();
    m_cube->vertices.resize(36);
    m_cube->matGroups.push_back(mg);
    
    Material *mat = new Material();
    mat->setOpaque(false);

    m_cubeMats = new MaterialArray();
    m_cubeMats->setMaterial(mg.matID, mat);
}

void RenderProgram::uploadCube()
{
    m_cube->upload(m_renderCtx);
    if(m_cube->vertexBuffer)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_cube->vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, m_cube->vertexBufferSize,
                     m_cube->vertices.constData(), GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    Material *mat = m_cubeMats->material(1);
    if(mat && !mat->texture())
    {
        uint32_t colorABGR = 0x66333366; // A=0.4, B=0.2, G=0.2, R=0.4
        texture_t texID = 0;
        uint32_t target = GL_TEXTURE_2D_ARRAY;
        glGenTextures(1, &texID);
        glBindTexture(target, texID);
        glTexImage3D(target, 0, GL_RGBA, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &colorABGR);
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(target, 0);
        mat->setTexture(texID);
        mat->setSubTexture(0);
    }
}

void RenderProgram::drawBox(const AABox &box)
{
    vec3 size = box.high - box.low;
    m_renderCtx->pushMatrix();
    m_renderCtx->translate(box.low.x, box.low.y, box.low.z);
    m_renderCtx->scale(size.x, size.y, size.z);
    m_renderCtx->translate(0.5, 0.5, 0.5);
    fromEightCorners(m_cube, cubeVertices);
    uploadCube();
    
    RenderBatch batch(m_cube, m_cubeMats);
    drawMesh(batch);
    m_renderCtx->popMatrix();
}

void RenderProgram::drawFrustum(const Frustum &frustum)
{
    fromEightCorners(m_cube, frustum.corners());
    uploadCube();
    
    RenderBatch batch(m_cube, m_cubeMats);
    drawMesh(batch);
}

////////////////////////////////////////////////////////////////////////////////

TextureSkinningProgram::TextureSkinningProgram(RenderContext *renderCtx) : RenderProgram(renderCtx)
{
}

TextureSkinningProgram::~TextureSkinningProgram()
{
}

bool TextureSkinningProgram::init()
{

    int32_t texUnits = 0;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &texUnits);
    if(texUnits < 2)
    {
        fprintf(stderr, "error: vertex texture fetch is not supported.\n");
        return false;
    }
    else if(!GLEW_ARB_texture_float)
    {
        fprintf(stderr, "error: extension 'ARB_texture_float' is not supported.\n");
        return false;
    }
    else if(!GLEW_ARB_texture_buffer_object_rgb32)
    {
        fprintf(stderr, "error: extension 'ARB_texture_buffer_object_32' is not supported.\n");
        return false;
    }
    return true;
}

void TextureSkinningProgram::beginSkinMesh(RenderBatch &batch)
{
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER_ARB, batch.animTex);
    glUniform1i(m_uniform[U_ANIM_TEX], 2);
    m_textureBinds++;
    glActiveTexture(GL_TEXTURE0);
}

void TextureSkinningProgram::skinMeshInstance(RenderBatch &batch, unsigned instanceID)
{
    uint32_t animID = batch.animIDs ? batch.animIDs[instanceID] : 0;
    float animFrame = batch.animFrames ? batch.animFrames[instanceID] : 0.0f;
    vec3 animTexSize = batch.animArray->textureDim();
    glUniform3fv(m_uniform[U_ANIM_TEX_SIZE], 1, (const GLfloat *)&animTexSize);
    glUniform1f(m_uniform[U_ANIM_ID], animID);
    glUniform1f(m_uniform[U_ANIM_FRAME], animFrame);
}

void TextureSkinningProgram::endSkinMesh(RenderBatch &batch)
{
    // restore state
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER_ARB, 0);
    glActiveTexture(GL_TEXTURE0);
}

////////////////////////////////////////////////////////////////////////////////

RenderBatch::RenderBatch()
{
    clear();
}

RenderBatch::RenderBatch(const MeshBuffer *meshBuf, MaterialArray *materials)
{
    clear();
    this->meshBuf = meshBuf;
    this->materials = materials;
}

void RenderBatch::clear()
{
    meshBuf = NULL;
    materials = NULL;
    animArray = NULL;
    animTex = 0;
    mvMatrices = NULL;
    colorSegments = NULL;
    materialMaps = NULL;
    sharedMaterialMap = NULL;
    animIDs = NULL;
    animFrames = NULL;
    pending = false;
    groups.clear();
    mergedGroups.clear();
    groupMats.clear();
    sharedTexture = 0;
    allOpaque = true;
}

// Batch state.
void RenderBatch::setBuffer(const MeshBuffer *meshBuf)
{
    // Cannot change the mesh buffer between beginBatch/endBatch.
    if(!pending)
    {
        this->meshBuf = meshBuf;
    }
}

void RenderBatch::setMaterials(MaterialArray *materials)
{
    if(!pending)
    {
        this->materials = materials;
    }
}

void RenderBatch::setMaterialTexture(texture_t materialTex)
{
    if(!pending)
    {
        this->sharedTexture = materialTex;
    }
}

void RenderBatch::setAnimations(AnimationArray *animArray)
{
    if(!pending)
    {
        this->animArray = animArray;
    }
}

void RenderBatch::setAnimationTexutre(texture_t animTex)
{
    if(!pending)
    {
        this->animTex = animTex;
    }
}

// Per-instance state.
void RenderBatch::setModelViewMatrices(const matrix4 *mvMatrices)
{
    this->mvMatrices = mvMatrices;
}

void RenderBatch::setColorSegments(const BufferSegment *colorSegments)
{
    this->colorSegments = colorSegments;
}

void RenderBatch::setMaterialMaps(MaterialMap **materialMaps)
{
    this->materialMaps = materialMaps;
}

void RenderBatch::setAnimationIDs(const uint32_t *animIDs)
{
    this->animIDs = animIDs;
}

void RenderBatch::setAnimationFrames(const float *animFrames)
{
    this->animFrames = animFrames;
}

// Shared instance state.
void RenderBatch::setSharedMaterialMap(MaterialMap *materialMap)
{
    this->sharedMaterialMap = materialMap;
}
