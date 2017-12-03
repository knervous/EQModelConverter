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

#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QRunnable>
#include <QThreadPool>
#include <include/glew-1.9.0/include/GL/glew.h>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/dds.h"
#include "EQuilibre/Render/dxt.h"

Material::Material()
{
    m_origin = LowerLeft;
    m_texture = 0;
    m_subTexture = 0;
    m_subTextureCount = 0;
    m_duration = 0;
    m_opaque = true;
}

bool Material::isOpaque() const
{
    return m_opaque;
}

void Material::setOpaque(bool opaque)
{
    m_opaque = opaque;
}

int Material::width() const
{
    return m_images.size() ? m_images.at(0).width() : 0;
}

int Material::height() const
{
    return m_images.size() ? m_images.at(0).height() : 0;
}

const QVector<QImage> & Material::images() const
{
    return m_images;
}

void Material::setImages(const QVector<QImage> &newImages)
{
    m_images = newImages;
}

Material::OriginType Material::origin() const
{
    return m_origin;
}

void Material::setOrigin(Material::OriginType newOrigin)
{
    m_origin = newOrigin;
}

texture_t Material::texture() const
{
    return m_texture;
}

void Material::setTexture(texture_t texture)
{
    m_texture = texture;
}

uint Material::subTexture() const
{
    return m_subTexture;
}

void Material::setSubTexture(uint newID)
{
    m_subTexture = newID;
}

uint32_t Material::subTextureCount() const
{
    return m_subTextureCount;
}

void Material::setSubTextureCount(uint32_t count)
{
    m_subTextureCount = count;
}

uint32_t Material::duration() const
{
    return m_duration;
}

void Material::setDuration(uint32_t durationMs)
{
    m_duration = durationMs;
}

bool Material::loadTextureDDS(const char *data, size_t size, QImage &img)
{
    dds_header_t hdr;
    dds_pixel_format_t pf;
    const char *d = data;
    size_t left = size;
    if(left < sizeof(hdr))
        return false;
    memcpy(&hdr, d, sizeof(hdr));
    left -= sizeof(hdr);
    d += sizeof(hdr);

    if(memcmp(hdr.magic, "DDS ", 4) || (hdr.size != 124) ||
        !(hdr.flags & DDSD_PIXELFORMAT) || !(hdr.flags & DDSD_CAPS) )
        return false;
    if(!(hdr.flags & DDSD_LINEARSIZE) || (hdr.pitch_or_linsize > left))
        return false;

    pf = hdr.pixelfmt;
    if((pf.flags & DDPF_FOURCC))
    {
        int format = DDS_COMPRESS_NONE;
        if(memcmp(pf.fourcc, "DXT1", 4) == 0)
            format = DDS_COMPRESS_BC1;
        else if(memcmp(pf.fourcc, "DXT5", 4) == 0)
            format = DDS_COMPRESS_BC3;
        if(format != DDS_COMPRESS_NONE)
        {
            img = QImage(hdr.width, hdr.height, QImage::Format_ARGB32);
            dxt_decompress(img.bits(), (unsigned char *)d, format, left, hdr.width, hdr.height, 4);
            return true;
        }
    }
    qDebug("DDS format not supported / not implemented");
    return false;
}

////////////////////////////////////////////////////////////////////////////////

class TextureUpload : public QRunnable
{
public:
    TextureUpload(bool useFence);
    virtual ~TextureUpload();
    
    const QVector<Material *> & materials() const;
    int maxWidth() const;
    int maxHeight() const;
    bool useFence() const;
    fence_t fence() const;
    
    virtual void run();
    
    void addMaterials(const QVector<Material *> &materials);
    bool upload(RenderContext *renderCtx);
    bool checkPrepared();
    
    void clear(RenderContext *renderCtx);
    
private:
    void analyzeImages();
    void packTextures();
    texture_t loadTextures(RenderContext *renderCtx);
    
    buffer_t m_pixelBuffer;
    fence_t m_fence;

    QMutex m_lock;
    bool m_prepared;
    bool m_useGenMipmaps;
    bool m_seenBMP;
    bool m_seenDDS;
    bool m_useFence;
    QVector<Material *> m_materials;
    QVector<QImage> m_images;
    QVector<bool> m_isDDS;
    uint8_t *m_pixelData;
    size_t m_pixelSize;
    int m_maxWidth;
    int m_maxHeight;
    int m_maxLevel;
    std::vector<size_t> m_levelOffset;
    std::vector<size_t> m_levelSize;
    std::vector<int> m_layerWidth;
    std::vector<int> m_layerHeight;
};

TextureUpload::TextureUpload(bool useFence)
{
    m_useFence = useFence;
    m_prepared = false;
    m_useGenMipmaps = false;
    m_seenBMP = m_seenDDS = false;
    m_maxLevel = 0;
    m_maxWidth = m_maxHeight = 0;
    m_pixelSize = 0;
    m_pixelData = NULL;
    m_pixelBuffer = 0;
    m_fence = NULL;
}

TextureUpload::~TextureUpload()
{
    clear(NULL);
}

const QVector<Material *> & TextureUpload::materials() const
{
    return m_materials;
}

int TextureUpload::maxWidth() const
{
    return m_maxWidth;
}

int TextureUpload::maxHeight() const
{
    return m_maxHeight;
}

bool TextureUpload::useFence() const
{
    return m_useFence;
}

fence_t TextureUpload::fence() const
{
    return m_fence;
}

void TextureUpload::clear(RenderContext *renderCtx)
{
    m_materials.clear();
    m_images.clear();
    m_isDDS.clear();
    delete [] m_pixelData;
    m_pixelData = NULL;
    
    if(renderCtx && m_pixelBuffer)
    {
       renderCtx->freeBuffers(&m_pixelBuffer, 1);
       m_pixelBuffer = 0;
    }
    
    if(renderCtx && m_fence)
    {
        renderCtx->deleteFence(m_fence);
        m_fence = NULL;
    }
}

static void copyImageARGB32(const QImage &src, uint32_t *dst, size_t dstSize,
                            size_t slicePitch, uint32_t z, uint32_t repeatX,
                            uint32_t repeatY, bool invertY)
{
    uint32_t width = src.width(), height = src.height();
    int dstPos = (slicePitch * z);
    for(uint32_t i = 0; i < repeatY; i++)
    {
        for(uint32_t y = 0; y < height; y++)
        {
            int scanIndex = invertY ? (height - y - 1) : y;
            const QRgb *srcBits = (const QRgb *)src.scanLine(scanIndex);
            for(uint32_t j = 0; j < repeatX; j++)
            {
                int widthBytes = width * sizeof(QRgb);
                Q_ASSERT(((dstPos * sizeof(QRgb)) + widthBytes) <= dstSize);
                memcpy(dst + dstPos, srcBits, widthBytes);
                dstPos += width;
            }
        }
    }
}

static void copyImageIndexed8(const QImage &src, uint32_t *dst, size_t dstSize,
                              size_t slicePitch, uint32_t z, uint32_t repeatX,
                              uint32_t repeatY, bool invertY)
{
    uint32_t width = src.width(), height = src.height();
    int dstPos = (slicePitch * z);
    QVector<QRgb> colors = src.colorTable();
    for(uint32_t i = 0; i < repeatY; i++)
    {
        for(uint32_t y = 0; y < height; y++)
        {
            int scanIndex = invertY ? (height - y - 1) : y;
            const uint8_t *srcBits = (const uint8_t *)src.scanLine(scanIndex);
            for(uint32_t j = 0; j < repeatX; j++)
            {
                for(uint32_t x = 0; x < width; x++)
                {
                    Q_ASSERT(((dstPos + 1) * sizeof(QRgb)) <= dstSize);
                    uint8_t index = srcBits[x];
                    dst[dstPos++] = (index < colors.count()) ? colors.value(index) : 0;
                }
            }
        }
    }
}

static void copyImage(const QImage &src, uint32_t *dst, size_t dstSize,
                      size_t slicePitch, uint32_t z, uint32_t repeatX,
                      uint32_t repeatY, bool invertY)
{
    switch(src.format())
    {
    case QImage::Format_ARGB32:
        copyImageARGB32(src, dst, dstSize, slicePitch, z, repeatX, repeatY, invertY);
        break;
    case QImage::Format_Indexed8:
        copyImageIndexed8(src, dst, dstSize, slicePitch, z, repeatX, repeatY, invertY);
        break;
    default:
        qDebug("Unsupported pixel format: %d", src.format());
        Q_ASSERT(0 && "Unsupported pixel format");
        break;
    }
}

static uint32_t maxMipmapLevel(uint32_t texWidth, uint32_t texHeight, uint32_t maxRepeat)
{
    int width = texWidth / maxRepeat, height = texHeight / maxRepeat;
    uint32_t level = 0;
    while(true)
    {
        width >>= 1;
        height >>= 1;
        if((width == 0) && (height == 0))
            return level;
        level++;
    }
    return 0;
}

void TextureUpload::addMaterials(const QVector<Material *> &materials)
{
    QMutexLocker locker(&m_lock);
    uint32_t subTexID = 1;
    foreach(Material *mat, materials)
    {
        if(!mat)
            continue;
        uint32_t subTextures = 0;
        mat->setSubTexture(subTexID);
        foreach(QImage img, mat->images())
        {
            if(!img.isNull() && (mat->texture() == 0))
            {
                m_images.push_back(img);
                m_isDDS.push_back(mat->origin() == Material::LowerLeft);
                subTextures++;
            }
        }
        mat->setSubTextureCount(subTextures);
        subTexID += subTextures;
        m_materials.append(mat);
    }
    analyzeImages();
}

void TextureUpload::analyzeImages()
{
    // Figure out what's the maximum texture dimensions of the images.
    int count = m_images.size();
    m_maxWidth = m_maxHeight = 0;
    for(size_t i = 0; i < count; i++)
    {
        m_maxWidth = qMax(m_maxWidth, m_images[i].width());
        m_maxHeight = qMax(m_maxHeight, m_images[i].height());
    }

    // Figure out what's the maximum mipmap level we can use.
    int maxRepeat = 1;
    for(size_t i = 0; i < count; i++)
    {
        QImage img = m_images[i];
        int repeatX = m_maxWidth / img.width();
        int repeatY = m_maxHeight / img.height();
        maxRepeat = qMax(maxRepeat, qMax(repeatX, repeatY));
    }
    m_maxLevel = maxMipmapLevel(m_maxWidth, m_maxHeight, maxRepeat);
    
    // Figure out the format of the images.
    m_seenDDS = m_seenBMP = false;
    for(size_t i = 0; i < count; i++)
    {
        if(m_isDDS[i])
            m_seenDDS = true;
        else
            m_seenBMP = true;
    }
    
    // Create each mipmap level of the texture.
    int maxGenLevel = m_useGenMipmaps ? 0 : m_maxLevel;
    m_pixelSize = 0;
    m_layerWidth.resize(maxGenLevel + 1);
    m_layerHeight.resize(maxGenLevel + 1);
    m_levelOffset.resize(maxGenLevel + 1);
    m_levelSize.resize(maxGenLevel + 1);
    for(int level = 0; level <= maxGenLevel; level++)
    {
        int layerWidth = qMax(m_maxWidth >> level, 1);
        int layerHeight = qMax(m_maxHeight >> level, 1);
        size_t levelSize = layerWidth * layerHeight * count * sizeof(uint32_t);
        m_layerWidth[level] = layerWidth;
        m_layerHeight[level] = layerHeight;
        m_levelOffset[level] = m_pixelSize;
        m_levelSize[level] = levelSize;
        m_pixelSize += levelSize;
    }   
}

void TextureUpload::run()
{
    m_lock.lock();
    m_prepared = false;
    m_lock.unlock();
    packTextures();
    m_lock.lock();
    m_prepared = true;
    m_lock.unlock();
}

void TextureUpload::packTextures()
{
    // Copy all images to a single image for each mipmap level.
    m_pixelData = new uint8_t[m_pixelSize];
    for(int level = 0; level < m_levelOffset.size(); level++)
    {
        int layerWidth = m_layerWidth[level];
        int layerHeight = m_layerHeight[level];
        size_t slicePitch = layerWidth * layerHeight;
        uint32_t *levelBits = (uint32_t *)(m_pixelData + m_levelOffset[level]);
        for(size_t i = 0; i < m_images.count(); i++)
        {
            // Scale down the original image to generate the mipmap.
            QImage img = m_images[i];
            int origWidth = img.width(), origHeight = img.height();
            int scaledWidth = (origWidth >> level), scaledHeight = (origHeight >> level);
            Q_ASSERT((scaledWidth > 0) || (scaledHeight > 0));
            scaledWidth = qMax(scaledWidth, 1);
            scaledHeight = qMax(scaledHeight, 1);
            if(level > 0)
                img = img.scaled(scaledWidth, scaledHeight);
            
            // Repeat textures smaller than the texture array
            // so that we can easily use GL_REPEAT.            
            uint32_t repeatX = qMin(m_maxWidth / origWidth, layerWidth);
            uint32_t repeatY = qMin(m_maxHeight / origHeight, layerHeight);
            bool flipY = !m_isDDS[i];
            copyImage(img, levelBits, m_levelSize[level], slicePitch, i,
                      repeatX, repeatY, flipY);
        }
    }
}

bool TextureUpload::upload(RenderContext *renderCtx)
{
    texture_t tex = loadTextures(renderCtx);
    for(uint i = 0; i < m_materials.count(); i++)
    {
        m_materials[i]->setTexture(tex);
    }
    return (tex != 0);
}

texture_t TextureUpload::loadTextures(RenderContext *renderCtx)
{
    if(!renderCtx || !renderCtx->isValid())
    {
        return 0;
    }

    GLuint target = GL_TEXTURE_2D_ARRAY;
    texture_t texID = 0;

    glGenTextures(1, &texID);
    glBindTexture(target, texID);
    
    // Create each mipmap level of the texture.
    int layers = m_layerWidth.size();
    for(int level = 0; level < layers; level++)
    {
        glTexImage3D(target, level, GL_RGBA, m_layerWidth[level], m_layerHeight[level],
                     m_images.size(), 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    }
    
    // Allocate a buffer for the image data.
    glGenBuffers(1, &m_pixelBuffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pixelBuffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_pixelSize, NULL, GL_STATIC_DRAW);
    uint8_t *bufferBits = (uint8_t *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(!bufferBits)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glDeleteTextures(0, &texID);
        return 0;
    }
    memcpy(bufferBits, m_pixelData, m_pixelSize);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    // Upload the image data.
    for(int level = 0; level < layers; level++)
    {

        glTexSubImage3D(target, level, 0, 0, 0, m_layerWidth[level], m_layerHeight[level],
                        m_images.size(), GL_BGRA, GL_UNSIGNED_BYTE, (void *)m_levelOffset[level]);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    if(m_useFence)
    {
        m_fence = renderCtx->createFence();
    }
    
    // Set texture parameters.
    if(m_seenDDS)
    {
        GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA};
        glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        if(m_seenBMP)
        {
            qDebug("FIXME: mixed BMP and DDS textures. "
                   "The DDS textures will have incorrect channels.");
        }
    }
    glTexParameterf(target, GL_TEXTURE_MAX_LEVEL, m_maxLevel);
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if(m_useGenMipmaps)
        glGenerateMipmapEXT(GL_TEXTURE_2D_ARRAY);
    glBindTexture(target, 0);
    return texID;
}

bool TextureUpload::checkPrepared()
{
    QMutexLocker locker(&m_lock);
    return m_prepared;
}

////////////////////////////////////////////////////////////////////////////////

MaterialArray::MaterialArray()
{
    m_state = eUploadNotStarted;
    m_upload = NULL;
    m_arrayTexture = 0;
    m_maxWidth = m_maxHeight = 0;
}

MaterialArray::~MaterialArray()
{
    clear(NULL);
}

const QVector<Material *> & MaterialArray::materials() const
{
    return m_materials;
}

Material * MaterialArray::material(uint32_t matID) const
{
    return (matID < m_materials.size()) ? m_materials.value(matID) : NULL;
}

void MaterialArray::setMaterial(uint32_t matID, Material *mat)
{
    if(matID >= m_materials.size())
        m_materials.resize(matID + 1);
    m_materials[matID] = mat;
}

texture_t MaterialArray::arrayTexture() const
{
    return m_arrayTexture;
}

UploadState MaterialArray::state() const
{
    return m_state;
}

int MaterialArray::maxWidth() const
{
    return m_maxWidth;
}

int MaterialArray::maxHeight() const
{
    return m_maxHeight;
}

void MaterialArray::textureArrayInfo(int &maxWidth, int &maxHeight,
                                   size_t &totalMem, size_t &usedMem) const
{
    const size_t pixelSize = 4;
    size_t count = 0;
    maxWidth = maxHeight = totalMem = usedMem = 0;
    foreach(Material *mat, m_materials)
    {
        if(!mat)
            continue;
        foreach(QImage img, mat->images())
        {
            int width = img.width();
            int height = img.height();
            maxWidth = qMax(maxWidth, width);
            maxHeight = qMax(maxHeight, height);
            usedMem += (width * height * pixelSize);
            //qDebug("Texture %d x %d", width, height);
            count++;
        }
    }
    totalMem = maxWidth * maxHeight * pixelSize * count;
}

void MaterialArray::clear(RenderContext *renderCtx)
{
    foreach(Material *mat, m_materials)
    {
        if(!m_arrayTexture && renderCtx && mat && mat->texture())
        {
            renderCtx->freeTexture(mat->texture());
        }
        delete mat;
    }
    m_materials.clear();
    
    if(m_arrayTexture && renderCtx)
    {
        renderCtx->freeTexture(m_arrayTexture);
    }
    m_arrayTexture = 0;
    
    if(m_upload)
    {
        m_upload->clear(renderCtx);
        delete m_upload;
        m_upload = NULL;
    }
    
    m_state = eUploadNotStarted;
}

void MaterialArray::uploadArray(RenderContext *renderCtx, bool useFence)
{
    if((m_state != eUploadNotStarted) || !renderCtx || !renderCtx->isValid())
        return;
    m_upload = new TextureUpload(useFence);
    m_upload->addMaterials(m_materials);
    m_maxWidth = m_upload->maxWidth();
    m_maxHeight = m_upload->maxHeight();
    m_state = eUploadPreparing;
    m_upload->setAutoDelete(false);
    QThreadPool::globalInstance()->start(m_upload);
}

UploadState MaterialArray::checkUpload(RenderContext *renderCtx)
{
    if(m_state == eUploadPreparing && m_upload->checkPrepared())
    {
        m_upload->upload(renderCtx);
        m_state = eUploadInProgress;
    }
    
    if((m_state == eUploadInProgress) &&
       (!m_upload->useFence() || renderCtx->isFenceSignaled(m_upload->fence())))
    {
        m_upload->clear(renderCtx);
        m_state = eUploadFinished;
    }
    return m_state;
}

////////////////////////////////////////////////////////////////////////////////

size_t MaterialMap::count() const
{
    return m_mappings.count();
}

uint32_t * MaterialMap::mappings()
{
    return m_mappings.data();
}

const uint32_t * MaterialMap::mappings() const
{
    return m_mappings.constData();
}

uint32_t * MaterialMap::offsets()
{
    return m_offsets.data();
}

const uint32_t * MaterialMap::offsets() const
{
    return m_offsets.constData();
}

uint32_t MaterialMap::mappingAt(size_t index) const
{
    return (index < m_mappings.count()) ? m_mappings[index] : index;
}

void MaterialMap::setMappingAt(size_t index, uint32_t mapping)
{
    m_mappings[index] = mapping;
}

uint32_t MaterialMap::offsetAt(size_t index) const
{
    return (index < m_offsets.count()) ? m_offsets[index] : 0;
}

void MaterialMap::setOffsetAt(size_t index, uint32_t offset)
{
    m_offsets[index] = offset;
}

void MaterialMap::resize(size_t newCount)
{
    size_t oldCount = count();
    m_mappings.resize(newCount);
    for(size_t i = oldCount; i < newCount; i++)
        m_mappings[i] = i;
    m_offsets.resize(newCount);
    for(size_t i = oldCount; i < newCount; i++)
        m_offsets[i] = 0;
}

void MaterialMap::clear()
{
    m_mappings.clear();
    m_offsets.clear();
}

void MaterialMap::fillTextureMap(MaterialArray *materials, vec3 *textureMap, size_t count) const
{
    int maxTexWidth = materials ? materials->maxWidth() : 0;
    int maxTexHeight = materials ? materials->maxHeight() : 0;
    unsigned mappingCount = this->count();
    for(size_t i = 0; i < count; i++)
    {
        uint32_t texID = (uint32_t)(i + 1);
        float matScalingX = 1.0, matScalingY = 1.0;
        if(i < mappingCount)
        {
            uint32_t matID = mappingAt(i);
            uint32_t texOffset = offsetAt(i);
            Material *mat = materials ? materials->material(matID) : NULL;
            if(mat)
            {
                matScalingX = (float)mat->width() / (float)maxTexWidth;
                matScalingY = (float)mat->height() / (float)maxTexHeight;
                texID = mat->subTexture() + texOffset;
            }
            else
            {
                texID = matID + texOffset;
            }
        }
        textureMap[i] = vec3(matScalingX, matScalingY, (float)texID);
    }
}
