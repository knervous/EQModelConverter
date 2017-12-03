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

#ifndef EQUILIBRE_UI_CHARACTER_SCENE_H
#define EQUILIBRE_UI_CHARACTER_SCENE_H

#include <QVector>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/Character.h"
#include "EQuilibre/UI/Scene.h"

class Camera;
class CharacterPack;
class Game;
class RenderProgram;
struct SpawnInfo;
class CharacterActor;
class CharacterModel;

struct  ActorInfo
{
    ActorInfo();
    void fill();
    
    RaceID raceID;
    GenderID genderID;
    QString modelName;
    QString displayName;
    CharacterModel *model;
    CharacterPack *pack;
};

class  CharacterScene : public Scene
{
public:
    CharacterScene(Game *game);
    virtual ~CharacterScene();
    
    enum SkinningMode
    {
        SoftwareSkinning = 0,
        HardwareSkinning = 1
    };
    
    enum RenderMode
    {
        Basic = 0,
        Skinning = 1
    };

    void init();

    Game * game() const;
    SkinningMode skinningMode() const;
    void setSkinningMode(SkinningMode newMode);
    
    int selectedActor() const;
    QString selectedAnimName() const;
    void setSelectedAnimName(QString newName);
    
    const QVector<ActorInfo> & actors() const;
    CharacterActor * actor() const;
    
    CharacterPack * loadCharacters(QString archivePath);
    void fillActorInfo(ActorInfo &info);
    void clear();

    virtual void update();
    virtual void draw();

    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);
    
signals:
    void actorListUpdated();

public slots:
    void setSelectedActor(int actorIndex);
    void setSelectedSkinID(int skinID);

private:
    RenderProgram * program(RenderMode renderMode);
    void drawFrame();
    void updateActorList();
    
    Q_OBJECT
    QVector<ActorInfo> m_actors;
    int m_selectedActorIndex;
    Camera *m_camera;
    CharacterActor *m_player;
    QString m_animName;
    vec3 m_delta;
    vec3 m_theta;
    float m_sigma;
    Game *m_game;
    double m_lastUpdateTime;
    SkinningMode m_skinningMode;
    // viewer settings
    MouseState m_transState;
    MouseState m_rotState;
};

#endif
