// Copyright (C) 2013 PiB <pixelbound@gmail.com>
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

#ifndef EQUILIBRE_GAME_GAME_PACKS_H
#define EQUILIBRE_GAME_GAME_PACKS_H

#include <QMap>
#include <QString>
#include "EQuilibre/Core/Platform.h"

class BoneTransform;
class Game;
struct GameUpdate;
class MaterialArray;
class MeshBuffer;
class MeshData;
class MeshDefFragment;
class PFSArchive;
class Skeleton;
class CharacterActor;
class WLDMesh;
class CharacterModel;
class WLDData;

enum AssetLoadState
{
    eAssetNotLoaded = 0,
    eAssetLoaded,
    eAssetUploadingTextures,
    eAssetUploadingGeometry,
    eAssetUploaded
};

/*!
  \brief Holds the resources needed to render static objects.
  */
class  ObjectPack
{
public:
    ObjectPack(QString name, Game *game);
    virtual ~ObjectPack();
    
    QString name() const;
    const QMap<QString, WLDMesh *> & models() const;
    MeshBuffer * buffer() const;
    
    bool load(QString archivePath, QString wldName);
    bool upload();
    void update(const GameUpdate &gu);
    void clear();
    
private:
    Game *m_game;
    QString m_name;
    PFSArchive *m_archive;
    WLDData *m_wld;
    QMap<QString, WLDMesh *> m_models;
    QMap<QString, AssetLoadState> m_modelState;
    AssetLoadState m_state;
    fence_t m_uploadFence;
    double m_uploadStart;
    MeshBuffer *m_meshBuf;
};

/*!
  \brief Holds the resources needed to render characters.
  */
class  CharacterPack
{
public:
    CharacterPack(QString name, Game *game);
    virtual ~CharacterPack();
    
    QString name() const;
    const QMap<QString, CharacterModel *> models() const;
    
    bool load(QString archivePath, QString wldName);
    bool resolveAnimations();
    void clear();
    
private:
    void importSkeletons(WLDData *wld);
    void importCharacterPalettes(PFSArchive *archive, WLDData *wld);
    void importCharacters(PFSArchive *archive, WLDData *wld);
    bool findModelRaceGender(CharacterModel *model, QString modelName) const;
    Skeleton * findSkeleton(QString modelName) const;
    
    Game *m_game;
    QString m_name;
    PFSArchive *m_archive;
    WLDData *m_wld;
    QMap<QString, CharacterModel *> m_models;
    MeshDefFragment *m_emptyDef;
    // This contains all the frames found in the character WLD file.
    // SkeletonTracks hold pointers to this which means we cannot resize it.
    // This needs to be deleted after the skeletons and animations.
    // XXX Still needed after the animation texture work?
    BoneTransform *m_frames;
    uint32_t m_frameCount;
};

/*!
  \brief Holds global game resources like assets.
  */
class  GamePacks
{
public:
    GamePacks(Game *game);
    ~GamePacks();
    
    QList<ObjectPack *> objectPacks() const;
    QList<CharacterPack *> characterPacks() const;
    MeshBuffer * builtinOjectBuffer() const;
    MaterialArray * builtinMaterials() const;
    MeshData * capsule() const;
    
    bool loadBuiltinOjects(QString path);
    ObjectPack * loadObjects(QString archivePath, QString wldName = QString::null);
    CharacterPack * loadCharacters(QString archivePath, QString wldName = QString::null, bool own = true);
    CharacterModel * findCharacter(QString name);
    CharacterPack * findCharacterPack(QString actorName);
    ObjectPack * findObjectPack(QString name);
    void update(const GameUpdate &gu);
    void clear();
    
private:
    MeshData *loadBuiltinSTLMesh(QString path);
    
    Game *m_game;
    QList<ObjectPack *> m_objectPacks;
    QList<CharacterPack *> m_charPacks;
    MeshBuffer *m_builtinObjects;
    MaterialArray *m_builtinMats;
    MeshData *m_capsule;
};

#endif
