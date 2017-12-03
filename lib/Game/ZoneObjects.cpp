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
#include "EQuilibre/Game/ZoneObjects.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/GamePacks.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/ZoneActors.h"
#include "EQuilibre/Game/ZoneTerrain.h"
#include "EQuilibre/Core/Fragments.h"
#include "EQuilibre/Core/WLDData.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderProgram.h"

ZoneObjects::ZoneObjects(Zone *zone)
{
    m_zone = zone;
    m_state = eAssetNotLoaded;
    m_pack = NULL;
    m_objDefWld = 0;
    m_objectsStat = NULL;
    m_objectsStatGPU = NULL;
    m_drawnObjectsStat = NULL;
}

ZoneObjects::~ZoneObjects()
{
    unload();
}

const AABox & ZoneObjects::bounds() const
{
    return m_bounds;
}

AABox & ZoneObjects::bounds()
{
    return m_bounds;
}

AssetLoadState ZoneObjects::state() const
{
    return m_state;
}

QVector<ObjectActor *> & ZoneObjects::visibleObjects()
{
    return m_visibleObjects;
}

void ZoneObjects::unload()
{
    Game *game = m_zone->game();
    foreach(Actor *actor, m_objects)
        delete actor;
    m_objects.clear();
    delete m_pack;
    m_pack = NULL;
    delete m_objDefWld;
    m_objDefWld = NULL;
    m_state = eAssetNotLoaded;
}

bool ZoneObjects::load(QString path, QString name, PFSArchive *mainArchive)
{
    m_objDefWld = WLDData::fromArchive(mainArchive, "objects.wld");
    if(!m_objDefWld)
        return false;
    
    QString objMeshPath = QString("%1/%2_obj.s3d").arg(path).arg(name);
    QString objMeshFile = QString("%1_obj.wld").arg(name);
    QScopedPointer<ObjectPack> pack(new ObjectPack(objMeshFile, m_zone->game()));
    if(!pack->load(objMeshPath, objMeshFile))
        return false;
    m_pack = pack.take();
    importActors();
    m_state = eAssetLoaded;
    return true;
}

void ZoneObjects::importActors()
{
    // import actors through Actor fragments
    const QMap<QString, WLDMesh *> &models = m_pack->models();
    WLDFragmentArray<ActorFragment> actorFrags = m_objDefWld->table()->byKind<ActorFragment>();
    for(uint32_t i = 0; i < actorFrags.count(); i++)
    {
        ActorFragment *actorFrag = actorFrags[i];
        QString actorName = actorFrag->m_def.name().replace("_ACTORDEF", "");
        WLDMesh *model = models.value(actorName);
        if(model)
        {
            ObjectActor *actor = new ObjectActor(actorFrag, model, m_zone);
            actor->setPack(m_pack);
            m_bounds.extendTo(actor->boundsAA());
            m_objects.append(actor);
        }
        else
        {
            qDebug("Actor '%s' not found", actorName.toLatin1().constData());
        }
    }
}

void ZoneObjects::addTo(OctreeIndex *tree)
{
    foreach(ObjectActor *actor, m_objects)
        tree->add(actor);   
}

void ZoneObjects::resetVisible()
{
    m_visibleObjects.clear();
}

bool ZoneObjects::upload()
{
    // Copy objects' lighting colors to a GPU buffer.
    RenderContext *renderCtx = m_zone->game()->renderContext();
    if(m_pack->upload() && (m_state != eAssetUploaded))
    {
        MeshBuffer *meshBuf = m_pack->buffer();
        foreach(ObjectActor *actor, m_objects)
            actor->importColorData(meshBuf);
        meshBuf->colorBufferSize = meshBuf->colors.count() * sizeof(uint32_t);
        if(meshBuf->colorBufferSize > 0)
        {
            meshBuf->colorBuffer = renderCtx->createBuffer(meshBuf->colors.constData(), meshBuf->colorBufferSize);
            meshBuf->clearColors();
        }
        m_state = eAssetUploaded;
    }
    return (m_state == eAssetUploaded);
}

static bool meshLessThan(const ObjectActor *a, const ObjectActor *b)
{
    return a->mesh()->def() < b->mesh()->def();
}

static bool packLessThan(const ObjectActor *a, const ObjectActor *b)
{
    return a->pack() < b->pack();
}

ObjectActor * ZoneObjects::createObject(const ObjectInfo &info)
{
    QString objName = info.name;
    objName = objName.replace("_ACTORDEF", "");
    ObjectPack *pack = m_zone->game()->packs()->findObjectPack(objName);
    if(!pack)
    {
        qDebug("warning: could not find model for object '%s'.", objName.toLatin1().constData());
        return NULL;
    }
    WLDMesh *mesh = pack->models().value(objName);
    OctreeIndex *index = m_zone->actors()->index();
    ObjectActor *object = new ObjectActor(info, mesh, m_zone);
    object->setPack(pack);
    m_bounds.extendTo(object->boundsAA());
    m_objects.append(object);
    if(index)
    {
        index->add(object);
        object->setIndexed(true);
    }
    return object;
}

ObjectActor * ZoneObjects::createEquip(uint32_t modelID)
{
    QString objName = QString("IT%1").arg(modelID);
    ObjectPack *pack = m_zone->game()->packs()->findObjectPack(objName);
    if(!pack)
    {
        qDebug("warning: could not find model for object '%s'.", objName.toLatin1().constData());
        return NULL;
    }
    WLDMesh *mesh = pack->models().value(objName);
    ObjectActor *object = new ObjectActor(mesh, m_zone);
    object->setPack(pack);
    return object;
}

void ZoneObjects::update(const GameUpdate &gu)
{
    if(m_pack)
    {
        m_pack->update(gu);
    }
}

void ZoneObjects::draw(RenderProgram *prog)
{
    RenderContext *renderCtx = m_zone->game()->renderContext();
    if(!renderCtx->isValid())
        return;
    
    if(m_visibleObjects.size() == 0)
    {
        return;
    }
    
    // Render the objects, one batch per pack.
    std::vector<ObjectActor *> packObjects;
    unsigned i = 0;
    qSort(m_visibleObjects.begin(), m_visibleObjects.end(), packLessThan);
    do
    {
        ObjectPack *pack = m_visibleObjects[i]->pack();
        for(; i < m_visibleObjects.size(); i++)
        {
            ObjectActor *object = m_visibleObjects[i];
            if(object->pack() != pack)
            {
                break;
            }
            packObjects.push_back(object);
        }
        draw(prog, packObjects, pack);
        packObjects.clear();
    } while(i < m_visibleObjects.size());
}

void ZoneObjects::draw(RenderProgram *prog, std::vector<ObjectActor *> &objects, ObjectPack *pack)
{
    RenderContext *renderCtx = m_zone->game()->renderContext();
    if(!pack || (objects.size() == 0) || !pack->upload())
    {
        return;
    }
    
    // Sort the list of visible objects by mesh.
    qSort(objects.begin(), objects.end(), meshLessThan);
    
    // Draw one batch of objects per mesh.
    const bool enableColor = m_zone->game()->hasFlag(eGameLighting);
    MeshBuffer *meshBuf = pack->buffer();
    RenderBatch batch;
    std::vector<matrix4> mvMatrices;
    const matrix4 &camera = renderCtx->matrix(RenderContext::ModelView);
    unsigned i = 0;
    do
    {
        // Begin the mesh batch.
        WLDMesh *mesh = objects[i]->mesh();
        MaterialArray *materials = mesh->palette()->array();
        MaterialMap *materialMap = mesh->palette()->map();
        meshBuf->addMaterialGroups(mesh->data());
        batch.setBuffer(meshBuf);
        batch.setMaterials(materials);
        prog->beginBatch(batch);
        
        // Add all objects with the same mesh to the batch.
        for(; i < objects.size(); i++)    
        {
            ObjectActor *object = objects[i];
            if(object->mesh() != mesh)
            {
                break;
            }
            
            matrix4 mvMatrix = camera * object->modelMatrix();
            if(enableColor)
            {
                batch.setModelViewMatrices(&mvMatrix);
                batch.setColorSegments(&object->colorSegment());
                batch.setMaterialMaps(&materialMap);
                prog->drawInstances(batch, 1);
            }
            else
            {
                mvMatrices.push_back(mvMatrix);
            }
        }
        
        // Flush the current batch.
        if(!enableColor)
        {
            batch.setModelViewMatrices(&mvMatrices[0]);
            batch.setColorSegments(NULL);
            batch.setSharedMaterialMap(materialMap);
            prog->drawInstances(batch, mvMatrices.size());
            mvMatrices.clear();
        }
        prog->endBatch(batch);
        meshBuf->matGroups.clear();
    } while(i < objects.size());
}
