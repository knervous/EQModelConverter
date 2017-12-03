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

#ifndef EQUILIBRE_RENDER_STATE_H
#define EQUILIBRE_RENDER_STATE_H

#include <QVector>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Core/Geometry.h"
//#include "EQuilibre/Render/FrameStat.h"

class QImage;
class BoneTransform;
class Frustum;
struct AABox;
class Material;
class MaterialArray;
class FrameStat;
class MeshBuffer;
class RenderContextPrivate;
class RenderProgram;

struct  LightParams
{
    vec3 color;
    Sphere bounds;
};

struct  FogParams
{
    float start;
    float end;
    float density;
    vec4 color;
};

enum ShaderID
{
    eShaderBasic = 0,
    eShaderSkinning = 1,
    eShaderCount = 2
};

class  RenderContext
{
public:
    RenderContext();
    virtual ~RenderContext();

    bool init();
    
    bool isValid() const;
    RenderProgram * programByID(ShaderID shaderID) const;
    void setCurrentProgram(RenderProgram *prog);

    // matrix operations

    enum MatrixMode
    {
        ModelView,
        Projection
    };

    void setMatrixMode(MatrixMode newMode);

    void loadIdentity();
    void multiplyMatrix(const matrix4 &m);
    void pushMatrix();
    void popMatrix();

    void translate(const vec3 &v);
    void rotate(const vec4 &q);
    void scale(const vec3 &v);
    void translate(float dx, float dy, float dz);
    void rotateX(float angle);
    void rotateY(float angle);
    void rotateZ(float angle);
    void scale(float sx, float sy, float sz);

    matrix4 currentMatrix() const;
    matrix4 & matrix(RenderContext::MatrixMode mode);
    const matrix4 & matrix(RenderContext::MatrixMode mode) const;

    // general state operations

    float aspectRatio() const;
    int width() const;
    int height() const;
    void setupViewport(int width, int heigth);
    bool beginFrame(const vec4 &clearColor);
    void endFrame();
    
    void setDepthWrite(bool write);

    // material operations

    texture_t createTextureFloat4(size_t width, size_t height, size_t layers,
                                  const vec4 *data);
    texture_t createTextureFromBuffer(buffer_t buffer);
    void freeTexture(texture_t tex);
    
    // buffer operations
    
    buffer_t createBuffer(const void *data, size_t size);
    void freeBuffers(buffer_t *buffers, int count);
    
    // Synchronization
    
    fence_t createFence();
    void deleteFence(fence_t fence);
    bool isFenceSignaled(fence_t fence);


    
protected:
    RenderContextPrivate *d;
};

#endif
