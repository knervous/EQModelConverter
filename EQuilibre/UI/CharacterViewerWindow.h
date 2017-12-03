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

#ifndef EQUILIBRE_CHARACTER_VIEWER_WINDOW_H
#define EQUILIBRE_CHARACTER_VIEWER_WINDOW_H

#include <QMainWindow>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/UI/CharacterScene.h"

class QComboBox;
class QVBoxLayout;
class QAction;
class RenderProgram;
class SceneViewport;
class Skeleton;
class CharacterActor;
class CharacterModel;
class CharacterScene;
class CharacterPack;
class Game;
class Zone;

class  CharacterViewerWindow : public QMainWindow
{
public:
    CharacterViewerWindow(Game *game, QWidget *parent = 0);
    ~CharacterViewerWindow();

    bool loadCharacters(QString archivePath);

private slots:
    void initialized();
    void updateLists();
    void loadActor(int actorIndex);
    void loadSkin(int skinID);
    void loadAnimation(QString animName);
    void openArchive();
    void copyAnimations();
    void clear();
    void setSoftwareSkinning();
    void setHardwareSkinning();

private:
    void initMenus();
    void copyAnimations(Skeleton *toSkel, QString fromChar);
    void updateMenus();

    Q_OBJECT
    Game *m_game;
    SceneViewport *m_viewport;
    CharacterScene *m_scene;
    QComboBox *m_actorText;
    QComboBox *m_paletteText;
    QComboBox *m_animationText;
    QString m_lastDir;
    QAction *m_softwareSkinningAction;
    QAction *m_hardwareSkinningAction;
    QAction *m_showFpsAction;
};

#endif
