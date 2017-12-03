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

#ifndef EQUILIBRE_ZONE_VIEWER_WINDOW_H
#define EQUILIBRE_ZONE_VIEWER_WINDOW_H

#include <QDialog>
#include <QMainWindow>
#include <QMap>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Core/World.h"
#include "EQuilibre/Game/Game.h"

class QComboBox;
class QLineEdit;
class QDialogButtonBox;
class QVBoxLayout;
class QAction;
class ZoneScene;
class SceneViewport;
class Game;
class Zone;

class  ZoneViewerWindow : public QMainWindow
{
public:
    ZoneViewerWindow(Game *game, QWidget *parent = 0);
    ~ZoneViewerWindow();
    
    virtual void closeEvent(QCloseEvent *e);

    bool isClient() const;

private slots:
    void openArchive();
    void clearZone();
    void selectAssetDir();
    void quit();
    void handleZoneLoading();
    void handleZoneLoaded();
    void handleZoneUnloaded();
    void setNoLighting();
    void setBakedLighting();
    void setDebugVertexColor();
    void setDebugTextureFactor();
    void setDebugDiffuse();


private:
    void initMenus();
    void updateMenus();
    QAction *createGameFlagAction(QString text, GameFlags flag);

    Q_OBJECT
    SceneViewport *m_viewport;
    Game *m_game;
    ZoneScene *m_scene;
    QMap<QString, ZoneInfo> m_zones;
    QAction *m_noLightingAction;
    QAction *m_bakedLightingAction;
    QAction *m_debugVertexColorAction;
    QAction *m_debugTextureFactorAction;
    QAction *m_debugDiffuseAction;
    QAction *m_showFpsAction;
    QAction *m_showZoneAction;
    QAction *m_showZoneObjectsAction;
    QAction *m_showZoneActorsAction;
    QAction *m_showSkyAction;
    QAction *m_showFogAction;
    QAction *m_cullZoneObjectsAction;
    QAction *m_showSoundTriggersAction;
    QAction *m_gpuSkinningAction;
};

class  GotoZoneDialog : public QDialog
{
public:
    GotoZoneDialog(Game *game, QWidget *parent);
    
    uint16_t destZoneID() const;
    void setDestZoneID(uint16_t zoneID);
    
    vec3 destPos() const;
    void setDestPos(const vec3 &newPos);
    
private:
    Game *m_game;
    QComboBox *m_destZoneText;
    QLineEdit *m_coordX;
    QLineEdit *m_coordY;
    QLineEdit *m_coordZ;
    QDialogButtonBox *m_buttons;
};

#endif
