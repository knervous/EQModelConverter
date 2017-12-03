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

#ifndef EQUILIBRE_RENDER_VERTEX_H
#define EQUILIBRE_RENDER_VERTEX_H

#include <QVector>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/LinearMath.h"

class MeshBuffer;
class MaterialArray;
class RenderContext;

class  Vertex
{
public:
    vec3 position;
    vec3 normal;
    vec3 texCoords;
    uint32_t color;
    uint32_t bone;
    uint32_t padding[1]; // align on 16-bytes boundaries
};

class  MaterialGroup
{
public:
    uint32_t id;
    uint32_t offset;
    uint32_t count;
    uint32_t matID;
};

struct  BufferSegment
{
    uint32_t elementSize;
    uint32_t offset;
    uint32_t count;
    
    BufferSegment();
    size_t size() const;
    size_t address() const;
};

class  MeshData
{
public:
    MeshData(MeshBuffer *buffer, uint32_t groups);
    ~MeshData();
    void updateTexCoords(MaterialArray *array, bool useMap = false);
    
    MeshBuffer *buffer;
    BufferSegment vertexSegment;
    BufferSegment indexSegment;
    MaterialGroup *matGroups;
    uint32_t groupCount;
};

class  MeshBuffer
{
public:
    MeshBuffer();
    ~MeshBuffer();
    MeshData *createMesh(uint32_t groups);
    void addMaterialGroups(MeshData *mesh);
    void updateTexCoords(MaterialArray *array, const MaterialGroup *matGroups,
                         uint32_t groupCount, uint32_t startIndex, bool useMap);
    void upload(RenderContext *renderCtx);
    void clear(RenderContext *renderCtx);
    void clearVertices();
    void clearIndices();
    void clearColors();
    
    QVector<Vertex> vertices;
    QVector<uint32_t> indices;
    QVector<uint32_t> colors;
    QVector<MaterialGroup> matGroups;
    QVector<MeshData *> meshes;
    buffer_t vertexBuffer;
    buffer_t indexBuffer;
    buffer_t colorBuffer;
    uint32_t vertexBufferSize;
    uint32_t indexBufferSize;
    uint32_t colorBufferSize;
    bool uploaded;
};

#endif
