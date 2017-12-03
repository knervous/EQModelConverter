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

#ifndef EQUILIBRE_UI_ZONE_SCENE_H
#define EQUILIBRE_UI_ZONE_SCENE_H

#include <QPointF>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/UI/Scene.h"
#include "EQuilibre/Render/Vertex.h"

class Game;
class RenderContext;
class RenderProgram;
class SceneViewport;
class CharacterActor;
class Zone;

struct CharacterTag
{
    QString text;
    QPointF pos;
    float fontSize;
};

class  ZoneScene : public Scene
{
public:
    ZoneScene(Game *game);
    virtual ~ZoneScene();

    Game * game() const;
    int lightingMode() const;
    void setLightingMode(int newMode);
    
    void loadZone(QString path, QString name);


    virtual void init();
    virtual void update();
    virtual void draw();
    
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);
    
signals:
    void loading();
    void loaded();
    void unloaded();

public slots:
    // XXX One slot for all flags.
    void showZone(bool show);
    void showZoneObjects(bool show);
    void showZoneActors(bool show);
    void showSky(bool show);
    void showFog(bool show);
    void setFrustumCulling(bool enabled);
    void showSoundTriggers(bool show);
    void enableGPUSkinning(bool enabled);

    
private slots:
    void handleZoneLoading();
    void handleZoneLoaded();
    void handleZoneUnloaded();
    void handlePlayerEntered();
    void handlePlayerLeft();

private:
    void clearText();
    void drawFrame();
    void addLocLine(QString text);
    void updateTextLayout();
    void updateLocLayout();
    void updateLogLayout();
    void updateTagLayout(const CharacterTag &tag);
    
    Q_OBJECT
    Game *m_game;
    Zone *m_zone;
    CharacterActor *m_player;
    MouseState m_rotState;
    RenderProgram *m_staticProgram;
    RenderProgram *m_actorProgram;
    int m_lightingMode;
    QFont *m_locFont;
    QFont *m_logFont;
    QString m_locText;
    QVector<CharacterTag> m_tags;
};

#endif
