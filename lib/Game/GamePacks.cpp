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

#include <QFile>
#include <QFileInfo>
#include "EQuilibre/Game/GamePacks.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDMaterial.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/ZoneActors.h"
#include "EQuilibre/Core/Character.h"
#include "EQuilibre/Core/Fragments.h"
#include "EQuilibre/Core/PFSArchive.h"
#include "EQuilibre/Core/StreamReader.h"
#include "EQuilibre/Core/WLDData.h"
#include "EQuilibre/Render/Material.h"

ObjectPack::ObjectPack(QString name, Game *game)
{
    m_name = name;
    m_game = game;
    m_uploadStart = 0.0;
    m_state = eAssetNotLoaded;
    m_archive = NULL;
    m_wld = NULL;
    m_uploadFence = NULL;
    m_meshBuf = NULL;
}

ObjectPack::~ObjectPack()
{
    clear();
}

QString ObjectPack::name() const
{
    return m_name;
}

const QMap<QString, WLDMesh *> & ObjectPack::models() const
{
    return m_models;
}

MeshBuffer * ObjectPack::buffer() const
{
    return m_meshBuf;
}

void ObjectPack::clear()
{
    m_game->clearFence(m_uploadFence);
    foreach(WLDMesh *model, m_models)
    {
        MaterialArray *matArray = model->palette()->array();
        m_game->clearMaterials(matArray);
        delete model;
    }
    m_models.clear();
    m_game->clearBuffer(m_meshBuf);
    delete m_wld;
    m_wld = NULL;
    delete m_archive;
    m_archive = NULL;
}

bool ObjectPack::load(QString archivePath, QString wldName)
{
    m_archive = new PFSArchive(archivePath);
    m_wld = WLDData::fromArchive(m_archive, wldName);
    if(!m_wld)
        return false;

    // Import models through ActorDef fragments.
    WLDFragmentArray<ActorDefFragment> actorDefs = m_wld->table()->byKind<ActorDefFragment>();
    for(uint32_t i = 0; i < actorDefs.count(); i++)
    {
        ActorDefFragment *actorDef = actorDefs[i];
        WLDFragment *subModel = actorDef->m_models.value(0);
        if(!subModel)
            continue;
        MeshFragment *mesh = subModel->cast<MeshFragment>();
        if(!mesh)
        {
            if(subModel->kind() == HierSpriteFragment::KIND)
                qDebug("Hierarchical model in zone objects (%s)",
                       actorDef->name().toLatin1().constData());
            continue;
        }
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        if(!mesh->m_def)
            continue;
        WLDMesh *model = new WLDMesh(mesh->m_def, 0);
        WLDMaterialPalette *pal = model->importPalette(m_archive);
        pal->createArray();
        pal->createMap();
        m_models.insert(actorName, model);
    }
    
    m_state = eAssetLoaded;
    return true;
}

bool ObjectPack::upload()
{
    RenderContext *renderCtx = m_game->renderContext();
    if(!renderCtx || !renderCtx->isValid())
    {
        return false;
    }
    
    if(m_state == eAssetLoaded)
    {
        // Start uploading the textures and update the material subtextures.
        m_uploadStart = m_game->currentTime();
        foreach(QString name, m_models.keys())
        {
            WLDMesh *mesh = m_models.value(name);
            MaterialArray *materials = mesh->palette()->array();
            materials->uploadArray(renderCtx, false);
        }
        m_state = eAssetUploadingTextures;
        
        // Import vertices and indices for each mesh.
        m_meshBuf = new MeshBuffer();
        foreach(WLDMesh *mesh, m_models.values())
        {
            MaterialArray *materials = mesh->palette()->array();
            MeshData *meshData = mesh->importFrom(m_meshBuf);
            meshData->updateTexCoords(materials, true);
        }
        
        // Create the GPU buffers.
        m_meshBuf->upload(renderCtx);
        m_meshBuf->clearVertices();
        m_meshBuf->clearIndices();
        
        // Create a fence to know when uploading is done.
        m_uploadFence = renderCtx->createFence();
    }
    
    // Check for texture upload completion.
    if((m_state == eAssetUploadingTextures))
    {
        bool allUploaded = true;
        foreach(QString name, m_models.keys())
        {
            WLDMesh *mesh = m_models.value(name);
            MaterialArray *materials = mesh->palette()->array();
            UploadState state = materials->checkUpload(renderCtx);
            if(state != eUploadFinished)
            {
                allUploaded = false;
            }
        }
        
        if(allUploaded && renderCtx->isFenceSignaled(m_uploadFence))
        {
            m_state = eAssetUploaded;
            renderCtx->deleteFence(m_uploadFence);
            m_uploadFence = NULL;
            
            double uploadDuration = m_game->currentTime() - m_uploadStart;
            qDebug("Uploaded object pack '%s' in %f.",
                   m_name.toLatin1().constData(), uploadDuration);
        }
    }
   
    return (m_state == eAssetUploaded);
}

void ObjectPack::update(const GameUpdate &gu)
{
    foreach(WLDMesh *mesh, models().values())
        mesh->palette()->animate(gu.currentTime);
}

////////////////////////////////////////////////////////////////////////////////

CharacterPack::CharacterPack(QString name, Game *game)
{
    m_name = name;
    m_game = game;
    m_archive = NULL;
    m_wld = NULL;
    m_emptyDef = NULL;
    m_frames = NULL;
    m_frameCount = 0;
}

CharacterPack::~CharacterPack()
{
    clear();
}

QString CharacterPack::name() const
{
    return m_name;
}

const QMap<QString, CharacterModel *> CharacterPack::models() const
{
    return m_models;
}

void CharacterPack::clear()
{
    foreach(CharacterModel *model, m_models)
    {
        model->clear(m_game->renderContext());
        delete model->skeleton();
        delete model;
    }
    m_models.clear();
    delete [] m_frames;
    m_frames = NULL;
    m_frameCount = 0;
    delete m_emptyDef;
    m_emptyDef = NULL;
    delete m_wld;
    delete m_archive;
    m_wld = 0;
    m_archive = 0;
}

bool CharacterPack::load(QString archivePath, QString wldName)
{
    m_archive = new PFSArchive(archivePath);
    m_wld = WLDData::fromArchive(m_archive, wldName);
    if(!m_wld)
    {
        delete m_archive;
        m_archive = 0;
        return false;
    }

    importCharacters(m_archive, m_wld);
    importCharacterPalettes(m_archive, m_wld);
    importSkeletons(m_wld);
    return true;
}

void CharacterPack::importSkeletons(WLDData *wld)
{
    // count skeleton track frames
    WLDFragmentArray<TrackDefFragment> trackDefs = wld->table()->byKind<TrackDefFragment>();
    std::vector<BoneTrack> boneTracks(trackDefs.count());
    std::map<uint32_t, uint32_t> trackMap;
    m_frameCount = 0;
    for(uint32_t i = 0; i < trackDefs.count(); i++)
    {
        TrackDefFragment *trackDef = trackDefs[i];
        BoneTrack &boneTrack = boneTracks[i];
        boneTrack.name = trackDef->name();
        boneTrack.frameCount = trackDef->m_frames.count();
        m_frameCount += boneTrack.frameCount;
        trackMap[trackDef->ID()] = i;
    }
    
    // import skeleton tracks
    m_frames = new BoneTransform[m_frameCount];
    BoneTransform *current = m_frames;
    for(uint32_t i = 0; i < trackDefs.count(); i++)
    {
        TrackDefFragment *trackDef = trackDefs[i];
        BoneTrack &boneTrack = boneTracks[i];
        for(uint32_t j = 0; j < boneTrack.frameCount; j++)
            current[j] = trackDef->m_frames[j];
        boneTrack.frames = current;
        current += boneTrack.frameCount;
    }
    
    // import skeletons which contain the pose animation
    WLDFragmentArray<HierSpriteDefFragment> skelDefs = wld->table()->byKind<HierSpriteDefFragment>();
    for(uint32_t i = 0; i < skelDefs.count(); i++)
    {
        HierSpriteDefFragment *skelDef = skelDefs[i];
        QString actorName = skelDef->name().replace("_HS_DEF", "");
        CharacterModel *model = m_models.value(actorName);
        if(!model)
            continue;
        BoneTrackSet tracks;
        foreach(SkeletonNode node, skelDef->m_tree)
        {
            uint32_t trackID = trackMap[node.track->m_def->ID()];
            tracks.push_back(boneTracks[trackID]);
        }
        model->setSkeleton(new Skeleton(skelDef->m_tree, tracks, skelDef->m_boundingRadius));
    }

    // import other animations
    WLDFragmentArray<TrackFragment> tracks = wld->table()->byKind<TrackFragment>();
    for(uint32_t i = 0; i < tracks.count(); i++)
    {
        TrackFragment *track = tracks[i];
        QString animName = track->name().left(3);
        QString actorName = track->name().mid(3, 3);
        CharacterModel *model = m_models.value(actorName);
        if(!model)
            continue;
        Skeleton *skel = model->skeleton();
        if(skel && track->m_def)
        {
            uint32_t trackID = trackMap[track->m_def->ID()];
            skel->addTrack(animName, boneTracks[trackID]);
        }
    }
}

void CharacterPack::importCharacterPalettes(PFSArchive *archive, WLDData *wld)
{
    WLDFragmentArray<MaterialDefFragment> matDefs = wld->table()->byKind<MaterialDefFragment>();
    for(uint32_t i = 0; i < matDefs.count(); i++)
    {
        MaterialDefFragment *matDef = matDefs[i];
        if(matDef->handled())
            continue;
        QString charName, partName;
        uint32_t skinID = 0;
        if(WLDMaterialPalette::explodeName(matDef, charName, skinID, partName))
        {
            CharacterModel *model = m_models.value(charName);
            if(!model)
                continue;
            WLDMaterialPalette *palette = model->mainMesh()->palette();
            model->getSkinMask(skinID, true);
            WLDMaterialSlot *matSlot = palette->slotByName(partName);
            if(matSlot)
                matSlot->addSkinMaterial(skinID, matDef);
        }
    }
}

static bool findMainMesh(const QList<MeshFragment *> &meshes, const QString &actorName,
                         MeshFragment *&meshOut, MaterialPaletteFragment *&palDefOut)
{
    QString mainMeshName = actorName + "_DMSPRITEDEF";
    foreach(MeshFragment *mesh, meshes)
    {
        MeshDefFragment *meshDef = mesh->m_def;
        QString meshName = meshDef ? meshDef->name() : QString();
        if(((meshName == mainMeshName) || meshes.size() == 1))
        {
            meshOut = mesh;
            palDefOut = meshDef ? meshDef->m_palette : NULL;
            return true;
        }
    }
    
    // Special case for the 'Invisible Man' model which has no mesh definition.
    if((actorName == "IVM") && (meshes.size() > 0))
    {
        meshOut = meshes[0];
        palDefOut = NULL;
        return true;
    }
    return false;
}

static bool explodeMeshName(QString defName, QString &actorName,
                            QString &meshName, uint32_t &skinID)
{
    // e.g. defName == 'ELEHE00_DMSPRITEDEF'
    // 'ELE' : character
    // 'HE' : mesh
    // '00' : skin ID
    static QRegExp r("^(\\w{3})(.*)(\\d{2})_DMSPRITEDEF$");
    if(r.exactMatch(defName))
    {
        actorName = r.cap(1);
        meshName = r.cap(2);
        skinID = r.cap(3).toUInt();
        return true;
    }
    return false;
}

void CharacterPack::importCharacters(PFSArchive *archive, WLDData *wld)
{
    WLDFragmentArray<ActorDefFragment> actorDefs = wld->table()->byKind<ActorDefFragment>();
    for(uint32_t i = 0; i < actorDefs.count(); i++)
    {
        ActorDefFragment *actorDef = actorDefs[i];
        QString actorName = actorDef->name().replace("_ACTORDEF", "");
        QList<MeshFragment *> meshes = CharacterModel::listMeshes(actorDef);
        MeshFragment *mainMeshRef = NULL;
        MaterialPaletteFragment *mainPalette = NULL;
        if(!findMainMesh(meshes, actorName, mainMeshRef, mainPalette))
        {
            qDebug("Warning: could not find main mesh for actor '%s'.", actorName.toLatin1().constData());
            continue;
        }
        
        // Create an empty mesh definition if needed (e.g. for Invisible Man).
        MeshDefFragment *mainMeshDef = mainMeshRef->m_def;
        if(!mainMeshDef)
        {
            if(!m_emptyDef)
            {
                m_emptyDef = new MeshDefFragment();
                m_emptyDef->m_flags = 0;
                m_emptyDef->m_palette = NULL;
                m_emptyDef->m_param2[0] = 0;
                m_emptyDef->m_param2[1] = 0;
                m_emptyDef->m_param2[2] = 0;
                m_emptyDef->m_maxDist = 0.0f;
                m_emptyDef->m_size9 = 0;
            }
            mainMeshDef = m_emptyDef;
        }

        // Create the main mesh.
        WLDMesh *mainMesh = new WLDMesh(mainMeshDef, 0);
        WLDMaterialPalette *pal = mainMesh->importPalette(archive);
        CharacterModel *model = new CharacterModel(mainMesh);
        uint32_t skinID = 0;
        foreach(MeshFragment *mesh, meshes)
        {
            MeshDefFragment *meshDef = mesh->m_def;
            if(!meshDef || (meshDef == mainMeshDef))
                continue;
            Q_ASSERT(mainPalette == meshDef->m_palette);
            model->addPart(meshDef, skinID);
            pal->addMeshMaterials(meshDef, skinID);
        }
        foreach(WLDFragment *frag, actorDef->m_models)
        {
            switch(frag->kind())
            {
            case HierSpriteFragment::KIND:
            case MeshFragment::KIND:
                break;
            default:
                qDebug("Unknown model fragment kind (0x%02x) %s",
                       frag->kind(), actorName.toLatin1().constData());
                break;
            }
        }

        findModelRaceGender(model, actorName);
        m_models.insert(actorName, model);
    }
    
    // look for alternate meshes (e.g. heads)
    WLDFragmentArray<MeshDefFragment> meshDefs = wld->table()->byKind<MeshDefFragment>();
    for(uint32_t i = 0; i < meshDefs.count(); i++)
    {
        MeshDefFragment *meshDef = meshDefs[i];
        if(meshDef->handled())
            continue;
        QString actorName, meshName;
        uint32_t skinID = 0;
        explodeMeshName(meshDef->name(), actorName, meshName, skinID);
        CharacterModel *model = m_models.value(actorName);
        if(!model || meshName.isEmpty())
            continue;
        WLDMaterialPalette *pal = model->mainMesh()->palette();
        uint32_t defaultSkinMask = model->getSkinMask(0);
        const std::vector<WLDMesh *> &parts = model->meshes();
        for(unsigned i = 0; i < parts.size(); i++)
        {
            if(!model->isPartUsed(defaultSkinMask, i))
                continue;
            WLDMesh *part = parts[i];
            QString actorName2, meshName2;
            uint32_t skinID2 = 0;
            explodeMeshName(part->def()->name(), actorName2, meshName2, skinID2);
            if((meshName2 == meshName) && (skinID2 != skinID))
            {
                model->replacePart(meshDef, skinID, i);
                pal->addMeshMaterials(meshDef, skinID);
            }
        }
    }
}

bool CharacterPack::findModelRaceGender(CharacterModel *model, QString modelName) const
{
    RaceInfo info;
    for(int r = 0; r <= RACE_MAX; r++)
    {
        RaceID race = (RaceID)r;
        if(!info.findByID(race))
            continue;
        if(modelName == info.modelMale)
        {
            model->setRace(race);
            model->setGender(eGenderMale);
            model->setAnimBase(info.animMale);
            return true;
        }
        else if(modelName == info.modelFemale)
        {
            model->setRace(race);
            model->setGender(eGenderFemale);
            model->setAnimBase(info.animFemale);
            return true;
        }
    }
    return false;
}

bool CharacterPack::resolveAnimations()
{
    int misses = 0;
    bool allLoaded = true;
    foreach(CharacterModel *model, m_models)
    {
        // All characters should have skeletons, but who knows.
        Skeleton *toSkel = model->skeleton();
        if(!toSkel)
        {
            continue;
        }
        
        // Copy animations from the base model, if any.
        QString animBase = model->animBase();
        if(!animBase.isEmpty())
        {
            Skeleton *fromSkel = findSkeleton(animBase);
            if(fromSkel)
                toSkel->copyAnimationsFrom(fromSkel);
            else
                misses++;
        }
        
        // Load the animations into the model's animation array.
        const char *animName = NULL;
        const QMap<QString, Animation *> &skelAnims = toSkel->animations();
        Animation *animations[eAnimCount];
        AnimationArray *animArray = model->animations();
        for(int i = 0; i < (int)eAnimCount; i++)
        {
            animName = CharacterInfo::findAnimationName((CharacterAnimation)i);
            if(animName)
                animations[i] = skelAnims.value(animName);
            else
                animations[i] = NULL;
        }
        allLoaded &= animArray->load(animations, eAnimCount);
    }
    return (misses == 0) && allLoaded;
}

Skeleton * CharacterPack::findSkeleton(QString modelName) const
{
    CharacterModel *model = m_models.value(modelName);
    if(model && model->skeleton())
        return model->skeleton();
    
    CharacterPack *pack = m_game->packs()->findCharacterPack(modelName);
    if(pack)
    {
        model = pack->models().value(modelName);
        if(model)
            return model->skeleton();
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

GamePacks::GamePacks(Game *game)
{
    m_game = game;
    m_builtinObjects = NULL;
    m_builtinMats = NULL;
    m_capsule = NULL;
}

GamePacks::~GamePacks()
{
    clear();
}

QList<ObjectPack *> GamePacks::objectPacks() const
{
    return m_objectPacks;
}

QList<CharacterPack *> GamePacks::characterPacks() const
{
    return m_charPacks;
}

MeshBuffer * GamePacks::builtinOjectBuffer() const
{
    return m_builtinObjects;
}

MaterialArray * GamePacks::builtinMaterials() const
{
    return m_builtinMats;
}

MeshData * GamePacks::capsule() const
{
    return m_capsule;
}

void GamePacks::clear()
{
    m_game->clearBuffer(m_builtinObjects);
    delete m_builtinMats;
    m_builtinMats = NULL;
    foreach(ObjectPack *pack, m_objectPacks)
        delete pack;
    foreach(CharacterPack *pack, m_charPacks)
        delete pack;
    m_objectPacks.clear();
    m_charPacks.clear();
}

bool GamePacks::loadBuiltinOjects(QString path)
{
    QFile capsuleFile(QString("%1/capsule.stl").arg(path));
    if(!capsuleFile.exists())
    {
        return false;
    }
    
    m_builtinObjects = new MeshBuffer();
    m_capsule = loadBuiltinSTLMesh(capsuleFile.fileName());
    
    Material *mat = new Material();
    mat->setOpaque(false);
    m_builtinMats = new MaterialArray();
    m_builtinMats->setMaterial(1, mat);
    return true;
}

ObjectPack * GamePacks::loadObjects(QString archivePath, QString wldName)
{
    if(wldName.isNull())
    {
        QString baseName = QFileInfo(archivePath).baseName();
        wldName = baseName + ".wld";
    }
    
    // Do not load the pack again if it has been already loaded.
    foreach(ObjectPack *pack, m_objectPacks)
    {
        if(pack->name() == wldName)
        {
            return pack;
        }
    }
    
    ObjectPack *objPack = new ObjectPack(wldName, m_game);
    if(!objPack->load(archivePath, wldName))
    {
        delete objPack;
        return NULL;
    }
    m_objectPacks.append(objPack);
    return objPack;
}

CharacterPack * GamePacks::loadCharacters(QString archivePath, QString wldName, bool own)
{
    if(wldName.isNull())
    {
        QString baseName = QFileInfo(archivePath).baseName();
        if(baseName == "global_chr1")
            baseName = "global_chr";
        wldName = baseName + ".wld";
    }
    
    // Do not load the pack again if it has been already loaded.
    foreach(CharacterPack *pack, m_charPacks)
    {
        if(pack->name() == wldName)
        {
            return pack;
        }
    }
    
    QScopedPointer<CharacterPack> pack(new CharacterPack(wldName, m_game));
    if(!pack->load(archivePath, wldName))
        return NULL;
    if(own)
        m_charPacks.append(pack.data());
    pack->resolveAnimations();
    return pack.take();
}

CharacterModel * GamePacks::findCharacter(QString name)
{
    CharacterPack *pack = findCharacterPack(name);
    if(pack)
        return pack->models().value(name);
    return NULL;
}

CharacterPack * GamePacks::findCharacterPack(QString actorName)
{
    // Look inside the current's zone packs first.
    Zone *zone = m_game->zone();
    if(zone)
    {
        foreach(CharacterPack *pack, zone->actors()->characterPacks())
        {
            if(pack->models().contains(actorName))
                return pack;
        }
    }
    
    // Look inside the global packs if not found in the zone.
    foreach(CharacterPack *pack, m_charPacks)
    {
        if(pack->models().contains(actorName))
            return pack;
    }
    return NULL;
}

ObjectPack * GamePacks::findObjectPack(QString name)
{
    // Look inside the global packs.
    foreach(ObjectPack *pack, m_objectPacks)
    {
        if(pack->models().contains(name))
            return pack;
    }
    return NULL;
}

MeshData * GamePacks::loadBuiltinSTLMesh(QString path)
{
    QFile file(path);
    const qint64 headerSize = 84;
    qint64 size = file.size();
    if(!file.open(QFile::ReadOnly) || (size <  headerSize))
    {
        return NULL;
    }
    
    StreamReader reader(&file);
    uint32_t numTriangles = 0;
    vec3 normal;
    uint16_t attribSize = 0;
    uint32_t defaultColorABGR = 0xffffffff; // A=1, B=1, G=1, R=1
    
    // Skip header.
    file.seek(80);
    reader.unpackField('I', &numTriangles);

    qint64 triangleSize = (numTriangles * 50);
    if(size < (headerSize + triangleSize))
    {
        return NULL;
    }
    
    MeshBuffer *meshBuf = m_builtinObjects;
    MeshData *data = meshBuf->createMesh(1);
    MaterialGroup *group = &data->matGroups[0];
    QVector<Vertex> &vertices(meshBuf->vertices);
    QVector<uint32_t> &indices(meshBuf->indices);
    BufferSegment &dataLoc = data->vertexSegment;
    uint32_t vertexCount = numTriangles * 3;
    uint32_t vertexIndex = vertices.count();
    dataLoc.offset = vertexIndex;
    dataLoc.count = vertexCount;
    dataLoc.elementSize = sizeof(Vertex);
    group->id = 0;
    group->offset = 0;
    group->count = vertexCount;
    group->matID = 1;
    for(uint32_t i = 0; i < numTriangles; i++)
    {
        reader.unpackArray("f", 3, &normal);
        for(int j = 0; j < 3; j++)
        {
            Vertex v;
            reader.unpackArray("f", 3, &v.position);
            v.normal = normal;
            v.texCoords = vec3();
            v.color = defaultColorABGR;
            v.bone = 0;
            v.padding[0] = 0;
            vertices.append(v);
            indices.append((i * 3) + j);
        }
        reader.unpackField('H', &attribSize);
    }
    return data;
}

void GamePacks::update(const GameUpdate &gu)
{
    foreach(ObjectPack *pack, m_objectPacks)
        pack->update(gu);
}
