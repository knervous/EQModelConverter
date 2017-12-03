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

#include <cstdio>
#include <include/glew-1.9.0/include/GL/glew.h>
#include <QImage>
#include <QString>
#include <QMatrix4x4>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Render/Material.h"


class RenderContextPrivate
{
public:
    RenderContextPrivate();
    
    bool initShader(ShaderID shader, QString vertexFile, QString fragmentFile);
    
    bool valid;
    int width;
    int height;
    RenderContext::MatrixMode matrixMode;
    matrix4 matrix[2];
    std::vector<matrix4> matrixStack[2];
    RenderProgram *programs[eShaderCount];
    uint32_t currentProgram;
    QVector<FrameStat *> stats;
    int gpuTimers;
};

RenderContextPrivate::RenderContextPrivate()
{
    matrixMode = RenderContext::ModelView;
    valid = false;
    width = 0;
    height = 0;
    for(int i = 0; i < eShaderCount; i++)
        programs[i] = NULL;
    currentProgram = 0;
    gpuTimers = 0;
}

bool RenderContextPrivate::initShader(ShaderID shader, QString vertexFile, QString fragmentFile)
{
    RenderProgram *prog = programs[(int)shader];
    if(!prog->load(vertexFile, fragmentFile))
    {
        delete prog;
        programs[(int)shader] = NULL;
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

RenderContext::RenderContext()
{
    d = new RenderContextPrivate();
}

RenderContext::~RenderContext()
{
    if(d->valid)
    {
        for(unsigned i = 0; i < eShaderCount; i++)
            delete d->programs[i];
    }
    delete d;
}

bool RenderContext::isValid() const
{
    return d->valid;
}

RenderProgram * RenderContext::programByID(ShaderID shaderID) const
{
    if(!d->valid || (shaderID < eShaderBasic) || (shaderID >= eShaderCount))
        return NULL;
    return d->programs[(int)shaderID];
}

void RenderContext::setCurrentProgram(RenderProgram *prog)
{
    uint32_t name = prog ? prog->program() : 0;
    if(d->valid && (d->currentProgram != name))
    {
        glUseProgram(name);
        d->currentProgram = name;
    }
}

void RenderContext::setMatrixMode(RenderContext::MatrixMode newMode)
{
    d->matrixMode = newMode;
}

bool RenderContext::beginFrame(const vec4 &clearColor)
{
    if(!d->valid)
        return false;
    d->currentProgram = 0;
    RenderProgram *prog = programByID(eShaderBasic);
    bool shaderLoaded = prog && prog->loaded();
    glPushAttrib(GL_ENABLE_BIT);
    if(shaderLoaded)
        setCurrentProgram(prog);
    glEnable(GL_DEPTH_TEST);
    setMatrixMode(ModelView);
    pushMatrix();
    loadIdentity();
    if(shaderLoaded)
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    else
        glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    return shaderLoaded;
}

void RenderContext::endFrame()
{
    if(!d->valid)
        return;
    
    // Count draw calls and texture binds made by all programs.
    int totalDrawCalls = 0;
    int totalTextureBinds = 0;
    for(int i = 0; i < eShaderCount; i++)
    {
        RenderProgram *prog = d->programs[i];
        if(prog)
        {
            totalDrawCalls += prog->drawCalls();
            totalTextureBinds += prog->textureBinds();
            prog->resetFrameStats();
        }
    }
    
    // Reset state.
    setMatrixMode(ModelView);
    popMatrix();
    setCurrentProgram(NULL);
    glPopAttrib();
    glCullFace(GL_BACK);

    // Wait for the GPU to finish rendering the scene if we are profiling it.
    if(d->gpuTimers > 0)
        glFinish();

}

int RenderContext::width() const
{
    return d->width;
}

int RenderContext::height() const
{
    return d->height;
}

float RenderContext::aspectRatio() const
{
    return (d->height > 0) ? (float)d->width / (float)d->height : 0.0f;
}

void RenderContext::setupViewport(int w, int h)
{
    d->width = w;
    d->height = h;
    glViewport(0, 0, w, h);
}

void RenderContext::setDepthWrite(bool write)
{
    glDepthMask(write);
}

void RenderContext::loadIdentity()
{
    int i = (int)d->matrixMode;
    d->matrix[i].setIdentity();
}

void RenderContext::multiplyMatrix(const matrix4 &m)
{
    int i = (int)d->matrixMode;
    d->matrix[i] = d->matrix[i] * m;
}

void RenderContext::pushMatrix()
{
    int i = (int)d->matrixMode;
    d->matrixStack[i].push_back(d->matrix[i]);
}

void RenderContext::popMatrix()
{
    int i = (int)d->matrixMode;
    d->matrix[i] = d->matrixStack[i].back();
    d->matrixStack[i].pop_back();
}

void RenderContext::translate(const vec3 &v)
{
    multiplyMatrix(matrix4::translate(v.x, v.y, v.z));
}

void RenderContext::translate(float dx, float dy, float dz)
{
    multiplyMatrix(matrix4::translate(dx, dy, dz));
}

void RenderContext::rotateX(float angle)
 {
    multiplyMatrix(matrix4::rotate(angle, 1.0f, 0.0f, 0.0f));
}

void RenderContext::rotateY(float angle)
{
    multiplyMatrix(matrix4::rotate(angle, 0.0f, 1.0f, 0.0f));
}

void RenderContext::rotateZ(float angle)
{
    multiplyMatrix(matrix4::rotate(angle, 0.0f, 0.0f, 1.0f));
}

void RenderContext::scale(float sx, float sy, float sz)
{
    multiplyMatrix(matrix4::scale(sx, sy, sz));
}

void RenderContext::rotate(const vec4 &q)
{
    multiplyMatrix(matrix4::rotate(q));
}

matrix4 RenderContext::currentMatrix() const
{
    return d->matrix[(int)d->matrixMode];
}

matrix4 & RenderContext::matrix(RenderContext::MatrixMode mode)
{
    return d->matrix[(int)mode];
}

const matrix4 & RenderContext::matrix(RenderContext::MatrixMode mode) const
{
    return d->matrix[(int)mode];
}

texture_t RenderContext::createTextureFloat4(size_t width, size_t height,
                                             size_t layers, const vec4 *data)
{
    if(!d->valid)
        return 0;
    
    GLuint target = GL_TEXTURE_2D_ARRAY;
    texture_t texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(target, texID);
    glTexImage3D(target, 0, GL_RGBA32F, width, height, layers, 0,
                 GL_RGBA, GL_FLOAT, data);
    glTexParameterf(target, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glBindTexture(target, 0);
    return texID;
}

texture_t RenderContext::createTextureFromBuffer(buffer_t buffer)
{
    if(!d->valid)
        return 0;
    
    GLuint target = GL_TEXTURE_BUFFER_ARB;
    texture_t texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(target, texID);
    glTexBufferARB(target, GL_RGBA32F, buffer);
    glBindTexture(target, 0);
    return texID;
}

void RenderContext::freeTexture(texture_t tex)
{
    if(d->valid && (tex != 0))
        glDeleteTextures(1, &tex);
}

buffer_t RenderContext::createBuffer(const void *data, size_t size)
{
    if(!d->valid)
        return 0;
    buffer_t buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
}

void RenderContext::freeBuffers(buffer_t *buffers, int count)
{
    if(!buffers)
        return;
    glDeleteBuffers(count, buffers);
    memset(buffers, 0, sizeof(buffer_t) * count);
}

fence_t RenderContext::createFence()
{
    return glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void RenderContext::deleteFence(fence_t fence)
{
    glDeleteSync((GLsync)fence);
}

bool RenderContext::isFenceSignaled(fence_t fence)
{
    if(!fence)
    {
        return false;
    }
    
    int n = 0;
    int status = 0;
    glGetSynciv((GLsync)fence, GL_SYNC_STATUS, sizeof(uint32_t), &n, &status);
    return (status == GL_SIGNALED);
}

bool RenderContext::init()
{
    if(d->valid)
        return true;
    GLenum err = glewInit();
    if(GLEW_OK != err)
    {
        fprintf(stderr, "GLEW Error: %s", glewGetErrorString(err));
        return false;
    }
    else if(!GLEW_VERSION_2_0)
    {
        fprintf(stderr, "OpenGL 2.0 features not available.");
        return false;
    }
    else if(!GL_EXT_texture_array)
    {
        fprintf(stderr, "'GL_EXT_texture_array' extension not available.");
        return false;
    }
    else if(!GL_ARB_sync)
    {
        fprintf(stderr, "'GL_ARB_sync' extension not available.");
        return false;
    }
    
    d->programs[(int)eShaderBasic] = new RenderProgram(this);
    d->programs[(int)eShaderSkinning] = new TextureSkinningProgram(this);
    if(!d->initShader(eShaderBasic, "vertex.glsl", "fragment.glsl"))
        return false;
    if(!d->initShader(eShaderSkinning, "vertex_skinned_texture.glsl", "fragment.glsl"))
        return false;
    d->valid = true;

    return true;
}
