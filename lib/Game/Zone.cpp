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
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/CharacterActor.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/ZoneActors.h"
#include "EQuilibre/Game/ZoneObjects.h"
#include "EQuilibre/Game/ZoneTerrain.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Core/Fragments.h"
#include "EQuilibre/Core/PFSArchive.h"
#include "EQuilibre/Core/SoundTrigger.h"
#include "EQuilibre/Core/WLDData.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Render/Material.h"

Zone::Zone(Game *game) : QObject(NULL)
{
    m_game = game;
    m_terrain = NULL;
    m_objects = NULL;
    m_actors = NULL;
    m_info.clear();
    m_mainArchive = 0;
    m_mainWld = 0;
    m_loaded = false;
    m_collisionChecksStat = NULL;
    m_collisionChecks = 0;
    
    m_camera = new Camera(game);
    m_terrain = new ZoneTerrain(this);
    m_objects = new ZoneObjects(this);
    m_actors = new ZoneActors(this);
}

Zone::~Zone()
{
    unload();
    delete m_actors;
    delete m_objects;
    delete m_terrain;
    delete m_camera;
}

ZoneTerrain * Zone::terrain() const
{
    return m_terrain;
}

ZoneObjects * Zone::objects() const
{
    return m_objects;
}

ZoneActors * Zone::actors() const
{
    return m_actors;
}

const QVector<LightActor *> & Zone::lights() const
{
    return m_lights;   
}


const ZoneInfo & Zone::info() const
{
    return m_info;
}

uint16_t Zone::id() const
{
    return m_info.zoneID;
}

bool Zone::isLoaded() const
{
    return m_loaded;
}

Game * Zone::game() const
{
    return m_game;
}

CharacterActor * Zone::player() const
{
    return m_actors->player();
}

Camera * Zone::camera() const
{
    return m_camera;
}

bool Zone::load(QString path, QString name)
{
    // If we don't have it in the table, just fill in the name and use the defaults.
    ZoneInfo info;
    if(!info.findByName(name))
    {
        info.name = name;
    }
    return load(path, info);
}

bool Zone::load(QString path, const ZoneInfo &info)
{
    // Check if we have already loaded this zone.
    if(info.zoneID == m_info.zoneID)
    {
        m_info = info;
        return true;
    }
    
    // Unload any asset from a previously loaded zone.
    unload();
    
    // Load the main archive and WLD file.
    QString zonePath = QString("%1/%2.s3d").arg(path).arg(info.name);
    QString zoneFile = QString("%1.wld").arg(info.name);
    m_mainArchive = new PFSArchive(zonePath);
    if(!m_mainArchive->isOpen())
    {
        emit loadFailed(info.name);
        return false;
    }
    emit loading();
    
    m_mainWld = WLDData::fromArchive(m_mainArchive, zoneFile);
    
    // Load the zone's terrain.
    if(!m_terrain->load(m_mainArchive, m_mainWld))
    {
        emit loadFailed(info.name);
        return false;
    }
    
    // Load the zone's static objects.
    m_objects->bounds() = m_terrain->bounds();
    if(!m_objects->load(path, info.name, m_mainArchive))
    {
        emit loadFailed(info.name);
        return false;
    }
    
    OctreeIndex *index = m_actors->createIndex(m_objects->bounds());
    m_objects->addTo(index);
    
    // Load the zone's light sources.
    if(!importLightSources(m_mainArchive))
    {
        emit loadFailed(info.name);
        return false;
    }
    
    // Load the zone's characters.
    QString charPath = QString("%1/%2_chr.s3d").arg(path).arg(info.name);
    QString charFile = QString("%1_chr.wld").arg(info.name);
    m_actors->loadCharacters(charPath, charFile);
    
    // Load the zone's sound triggers.
    QString triggersFile = QString("%1/%2_sounds.eff").arg(path).arg(info.name);
    SoundTrigger::fromFile(m_soundTriggers, triggersFile);
    
    m_info = info;
    m_loaded = true;
    emit loaded();
    return true;
}

bool Zone::importLightSources(PFSArchive *archive)
{
    QScopedPointer<WLDData> wld(WLDData::fromArchive(archive, "lights.wld"));
    if(!wld.data())
        return false;
    uint16_t ID = 0;
    WLDFragmentArray<LightSourceFragment> lightFrags = wld->table()->byKind<LightSourceFragment>();
    OctreeIndex *index = m_actors->index();
    for(uint32_t i = 0; i < lightFrags.count(); i++)
    {
        LightActor *actor = new LightActor(lightFrags[i], ID++);
        m_lights.append(actor);
        index->add(actor);
    }
    return true;
}

void Zone::unload()
{
    if(m_loaded)
    {
        emit unloading();
    }
    
    foreach(LightActor *light, m_lights)
        delete light;
    foreach(SoundTrigger *trigger, m_soundTriggers)
        delete trigger;
    m_lights.clear();
    m_soundTriggers.clear();
    if(m_loaded)
    {
        m_actors->unloadActors();
    }
    m_actors->unload();
    m_objects->unload();
    m_terrain->unload();
    delete m_mainWld;
    delete m_mainArchive;
    m_mainWld = 0;
    m_mainArchive = 0;
    
    if(m_loaded)
    {
        emit unloaded();
        m_loaded = false;
    }
}

void Zone::frustumCullingCallback(Actor *actor, void *user)
{
    // XXX Only use the octree for characters (dynamic) actors.
    // For static actors we can use the zone's BSP tree or a separate octree.
    Zone *z = (Zone *)user;
    ObjectActor *object = actor->cast<ObjectActor>();
    if(object)
        z->objects()->visibleObjects().append(object);
}

void Zone::update(const GameUpdate &gu)
{
    CharacterActor *player = this->player();
    if(!isLoaded())
        return;
    
    RenderContext *renderCtx = m_game->renderContext();
    m_collisionChecks = 0;
    m_camera->setCharActor(player);
    m_camera->frustum().setAspect(renderCtx->aspectRatio());
    m_camera->frustum().setFarPlane(m_info.maxClip);
    m_camera->calculateViewFrustum();
    m_terrain->resetVisible();
    m_objects->resetVisible();
    m_actors->resetVisible();
    m_terrain->update(gu);
    m_objects->update(gu);
    m_actors->update(gu);
    
    // Find visible regions.
    Frustum &realFrustum(m_camera->realFrustum());
    uint32_t currentRegionID = m_terrain->findRegionID(realFrustum.eye());
    if(currentRegionID)
    {
        m_terrain->setCurrentRegionID(currentRegionID);
        m_terrain->showNearbyRegions(realFrustum);
    }
    else
    {
        m_terrain->showAllRegions(realFrustum);
    }
    
    // Find visible dynamic actors.
    std::vector<CharacterActor *> &visibleChars = m_actors->visibleActors();
    bool dumpChars = m_game->hasFlag(eGameFrameAction1);
    if(dumpChars)
    {
        qDebug("Visible characters:");
    }
    foreach(RegionActor *region, m_terrain->visibleRegions())
    {
        const std::vector<CharacterActor *> &regionChars = region->characters();
        for(unsigned i = 0; i < regionChars.size(); i++)
        {
            CharacterActor *character = regionChars[i];
            character->markEquipVisible();
            if(dumpChars)
            {
                const SpawnState &cur = character->currentState();
                qDebug("0x%x: %s (%f %f %f)", character->spawnID(),
                       character->name().toLatin1().constData(),
                       cur.position.x, cur.position.y, cur.position.z);
            }
        }
        visibleChars.insert(visibleChars.end(), regionChars.begin(), regionChars.end());
    }
    
    // Build a list of visible actors.
    OctreeIndex *index = m_actors->index();
    if(index)
        index->findVisible(realFrustum, frustumCullingCallback, this,
                           m_game->hasFlag(eGameCullObjects));
    
    m_actors->updateVisible(gu);

}

void Zone::playerEntry()
{
    m_actors->playerEntry();
    emit playerEntered();
}

void Zone::playerExit()
{
    m_actors->playerExit();
    emit playerLeft();
}

void Zone::newCollisionCheck()
{
    m_collisionChecks++;
}

void Zone::draw(RenderProgram *staticProg, RenderProgram *actorProg)
{
    RenderContext *renderCtx = m_game->renderContext();
    if(!renderCtx->isValid())
        return;
    
    // Setup the render program.
    vec4 ambientLight(0.4f, 0.4f, 0.4f, 1.0f);
    renderCtx->setCurrentProgram(staticProg);
    staticProg->setAmbientLight(ambientLight);
    
    FogParams fogParams;
    fogParams.color = m_info.fogColor;
    fogParams.start = m_info.fogMinClip;
    fogParams.end = m_info.fogMaxClip;
    fogParams.density = m_game->hasFlag(eGameShowFog) ? m_info.fogDensity * 0.01f : 0.0f;
    staticProg->setFogParams(fogParams);
    
    // Draw the sky first.
    ZoneSky *sky = m_game->sky();
    if(m_game->hasFlag(eGameShowSky) && sky)
        sky->draw(staticProg, this);
    
    renderCtx->pushMatrix();
    renderCtx->multiplyMatrix(m_camera->frustum().camera());

    // Draw the zone's terrain.
    if(m_game->hasFlag(eGameShowTerrain))
        m_terrain->draw(staticProg);
    
    // Draw the zone's static objects.
    if(m_game->hasFlag(eGameShowObjects))
        m_objects->draw(staticProg);
    
    // draw sound trigger volumes
    if(m_game->hasFlag(eGameShowSoundTriggers))
    {
        foreach(SoundTrigger *trigger, m_soundTriggers)
            staticProg->drawBox(trigger->bounds());
    }
    
    // Draw the viewing frustum and bounding boxes of visible zone parts. if frozen.
    if(m_game->hasFlag(eGameFrustumIsFrozen))
    {
        staticProg->drawFrustum(m_camera->frozenFrustum());
        //foreach(WLDZoneActor *actor, visibleZoneParts)
        //    prog->drawBox(actor->boundsAA);
        //foreach(WLDZoneActor *actor, visibleObjects)
        //    prog->drawBox(actor->boundsAA);
    }
    
    // Draw the zone's actors.
    if(m_game->hasFlag(eGameShowCharacters))
    {
        RenderProgram *prog = staticProg;
        if(m_game->hasFlag(eGameGPUSkinning))
        {
            prog = actorProg;
            renderCtx->setCurrentProgram(prog);
        }
        m_actors->draw(prog);
    }
    
    renderCtx->popMatrix();
}

void Zone::currentSoundTriggers(QVector<SoundTrigger *> &triggers) const
{
    vec3 playerPos = player()->location();
    foreach(SoundTrigger *trigger, m_soundTriggers)
    {
        if(trigger->bounds().contains(playerPos))
            triggers.append(trigger);
    }
}

////////////////////////////////////////////////////////////////////////////////

SkyDef::SkyDef()
{
    mainLayer = secondLayer = NULL;
}

ZoneSky::ZoneSky(Game *game)
{
    m_game = game;
    m_state = eAssetNotLoaded;
    m_skyArchive = NULL;
    m_skyWld = NULL;
    m_skyMaterials = NULL;
    m_skyBuffer = NULL;
}

ZoneSky::~ZoneSky()
{
    clear();
}

AssetLoadState ZoneSky::state() const
{
    return m_state;
}

void ZoneSky::clear()
{
    foreach(SkyDef def, m_skyDefs)
    {
        delete def.mainLayer;
        delete def.secondLayer;
    }
    m_skyDefs.clear();
    m_game->clearBuffer(m_skyBuffer);
    delete m_skyArchive;
    delete m_skyWld;
    delete m_skyMaterials;
    m_skyArchive = NULL;
    m_skyWld = NULL;
    m_skyMaterials = NULL;
    m_state = eAssetNotLoaded;
}

bool ZoneSky::load(QString path)
{
    QString archivePath = QString("%1/sky.s3d").arg(path);
    m_skyArchive = new PFSArchive(archivePath);
    m_skyWld = WLDData::fromArchive(m_skyArchive, "sky.wld");
    if(!m_skyWld)
    {
        delete m_skyArchive;
        m_skyArchive = NULL;
        return false;
    }
    
    QRegExp nameExp("^LAYER(\\d)(\\d)_DMSPRITEDEF$");
    WLDFragmentArray<MeshDefFragment> meshDefs = m_skyWld->table()->byKind<MeshDefFragment>();
    for(uint32_t i = 0; i < meshDefs.count(); i++)
    {
        MeshDefFragment *meshDef = meshDefs[i];
        QString name = meshDef->name();
        if(!nameExp.exactMatch(name))
            continue;
        uint32_t skyID = nameExp.cap(1).toInt();
        uint32_t skySubID = nameExp.cap(2).toInt();
        uint32_t layerID = (skyID * 10) + skySubID;
        if(m_skyDefs.size() < skyID)
            m_skyDefs.resize(skyID);
        SkyDef &def = m_skyDefs[skyID - 1];
        WLDMesh *mesh = new WLDMesh(meshDef, layerID);
        mesh->importPalette(m_skyArchive);
        if(skySubID == 1)
            def.mainLayer = mesh;
        else
            def.secondLayer = mesh;
    }
    
    // Import materials into a single material array.
    m_skyMaterials = new MaterialArray();
    for(size_t i = 0; i < m_skyDefs.size(); i++)
    {
        SkyDef &def = m_skyDefs[i];
        if(def.mainLayer)
            def.mainLayer->palette()->exportTo(m_skyMaterials);
        if(def.secondLayer)
            def.secondLayer->palette()->exportTo(m_skyMaterials);
    }
    m_state = eAssetLoaded;
    return true;
}

bool ZoneSky::upload()
{
    RenderContext *renderCtx = m_game->renderContext();
    if(!renderCtx || !renderCtx->isValid())
    {
        return false;
    }
    
    if(m_state == eAssetLoaded)
    {
        // Start uploading the textures and update the material subtextures.
        m_skyMaterials->uploadArray(renderCtx);
        m_state = eAssetUploadingTextures;
        
        // Import vertices and indices for each mesh.
        m_skyBuffer = new MeshBuffer();
        for(uint32_t i = 0; i < m_skyDefs.size(); i++)
        {
            WLDMesh *mainMesh = m_skyDefs[i].mainLayer;
            if(mainMesh)
            {
                uint32_t mainPaletteOffset = mainMesh->palette()->arrayOffset();
                MeshData *mainMeshData = mainMesh->importFrom(m_skyBuffer, mainPaletteOffset);
                mainMeshData->updateTexCoords(m_skyMaterials);
            }
            
            WLDMesh *secondMesh = m_skyDefs[i].secondLayer;
            if(secondMesh)
            {
                uint32_t secondPaletteOffset = secondMesh->palette()->arrayOffset();
                MeshData *secondMeshData = secondMesh->importFrom(m_skyBuffer, secondPaletteOffset);
                secondMeshData->updateTexCoords(m_skyMaterials);
            }
        }
        
        // Create the GPU buffers and free the memory used for vertices and indices.
        m_skyBuffer->upload(renderCtx);
        m_skyBuffer->clearVertices();
        m_skyBuffer->clearIndices();
    }
    
    // Check for texture upload completion.
    if((m_state == eAssetUploadingTextures) &&
       (m_skyMaterials->checkUpload(renderCtx) == eUploadFinished))
    {
        m_state = eAssetUploaded;
    }
   
    return (m_state == eAssetUploaded);
}

void ZoneSky::draw(RenderProgram *prog, Zone *zone)
{
    RenderContext *renderCtx = m_game->renderContext();
    uint32_t skyID = zone->info().skyID;
    if(!skyID || (skyID > m_skyDefs.size()))
        return;
    
    // Restrict the movement of the camera so that we don't see the edges of the sky dome.
    vec3 camRot = zone->camera()->lookOrient();
    matrix4 viewMat2 = matrix4::rotate(camRot.x *  1.00, 1.0, 0.0, 0.0) *
                       matrix4::rotate(camRot.y * -0.25, 0.0, 1.0, 0.0) *
                       matrix4::rotate(camRot.z *  1.00, 0.0, 0.0, 1.0);
    vec3 focus = viewMat2.map(vec3(0.0, 1.0, 0.0));
    matrix4 viewMat = matrix4::lookAt(vec3(0.0, 0.0, 0.0), focus, vec3(0.0, 0.0, 1.0));
    
    renderCtx->setDepthWrite(false);
    
    // Import material groups from the visible layer.
    m_skyBuffer->matGroups.clear();
    const SkyDef &def = m_skyDefs[skyID - 1];
    if(def.secondLayer)
        m_skyBuffer->addMaterialGroups(def.secondLayer->data());
    if(def.mainLayer)
        m_skyBuffer->addMaterialGroups(def.mainLayer->data());
    
    // Draw the visible layer.
    RenderBatch batch(m_skyBuffer, m_skyMaterials);
    batch.setModelViewMatrices(&viewMat);
    prog->drawMesh(batch);
    
    renderCtx->setDepthWrite(true);
}
