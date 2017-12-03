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

#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include "EQuilibre/UI/CharacterViewerWindow.h"
#include "EQuilibre/UI/CharacterScene.h"
#include "EQuilibre/Core/Skeleton.h"
#include "EQuilibre/UI/SceneViewport.h"
#include "EQuilibre/Game/CharacterActor.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/GamePacks.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/Zone.h"

CharacterViewerWindow::CharacterViewerWindow(Game *game, QWidget *parent) : QMainWindow(parent)
{
    m_game = game;
    m_scene = new CharacterScene(game);
    m_lastDir = game->assetPath();
    setWindowTitle("EQuilibre Character Viewer");
    m_viewport = new SceneViewport(m_scene);
    m_actorText = new QComboBox();
    m_paletteText = new QComboBox();
    m_animationText = new QComboBox();

    QWidget *centralWidget = new QWidget();

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_actorText);
    hbox->addWidget(m_paletteText);
    hbox->addWidget(m_animationText);

    QVBoxLayout *l = new QVBoxLayout(centralWidget);
    l->addLayout(hbox);
    l->addWidget(m_viewport);

    setCentralWidget(centralWidget);

    initMenus();
    updateLists();

    connect(m_scene, SIGNAL(actorListUpdated()), this, SLOT(updateLists()));
    connect(m_actorText, SIGNAL(activated(int)), this, SLOT(loadActor(int)));
    connect(m_paletteText, SIGNAL(activated(int)), this, SLOT(loadSkin(int)));
    connect(m_animationText, SIGNAL(activated(QString)), this, SLOT(loadAnimation(QString)));
    connect(m_viewport, SIGNAL(initialized()), this, SLOT(initialized()));
}

CharacterViewerWindow::~CharacterViewerWindow()
{
    m_game->clear();
    delete m_viewport;
    delete m_scene;
}

void CharacterViewerWindow::initialized()
{
    updateLists();
    m_viewport->setAnimation(true);
}

void CharacterViewerWindow::initMenus()
{
    QMenu *fileMenu = new QMenu(this);
    fileMenu->setTitle("&File");

    QAction *openAction = new QAction("&Open S3D archive...", this);
    openAction->setShortcut(QKeySequence::Open);
    QAction *copyAnimationsAction = new QAction("Copy Animations From Character...", this);
    QAction *clearAction = new QAction("Clear", this);
    clearAction->setShortcut(QKeySequence::Delete);
    QAction *quitAction = new QAction("&Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);

    fileMenu->addAction(openAction);
    fileMenu->addAction(copyAnimationsAction);
    fileMenu->addAction(clearAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QMenu *renderMenu = new QMenu(this);
    renderMenu->setTitle("&Render");

    m_softwareSkinningAction = new QAction("Software Skinning", this);
    m_hardwareSkinningAction = new QAction("Hardware Skinning", this);
    m_softwareSkinningAction->setCheckable(true);
    m_hardwareSkinningAction->setCheckable(true);
    QActionGroup *skinningActions = new QActionGroup(this);
    skinningActions->addAction(m_softwareSkinningAction);
    skinningActions->addAction(m_hardwareSkinningAction);

    m_showFpsAction = new QAction("Show stats", this);
    m_showFpsAction->setCheckable(true);

    renderMenu->addAction(m_softwareSkinningAction);
    renderMenu->addAction(m_hardwareSkinningAction);
    renderMenu->addAction(m_showFpsAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(renderMenu);

    updateMenus();

    connect(openAction, SIGNAL(triggered()), this, SLOT(openArchive()));
    connect(copyAnimationsAction, SIGNAL(triggered()), this, SLOT(copyAnimations()));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clear()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    connect(m_softwareSkinningAction, SIGNAL(triggered()), this, SLOT(setSoftwareSkinning()));
    connect(m_hardwareSkinningAction, SIGNAL(triggered()), this, SLOT(setHardwareSkinning()));
   // connect(m_showFpsAction, SIGNAL(toggled(bool)), m_viewport, SLOT(setShowStats(bool)));
}

void CharacterViewerWindow::openArchive()
{
    QString filePath;
    // OpenGL rendering interferes wtih QFileDialog
    m_viewport->setAnimation(false);
    filePath = QFileDialog::getOpenFileName(0, "Select a S3D file to open",
        m_lastDir, "S3D Archive (*.s3d)");
    if(!filePath.isEmpty())
    {
        m_lastDir = QFileInfo(filePath).dir().absolutePath();
        loadCharacters(filePath);
    }
    m_viewport->setAnimation(true);
}

bool CharacterViewerWindow::loadCharacters(QString archivePath)
{
    m_viewport->makeCurrent();
    if(m_scene->loadCharacters(archivePath))
    {
        return true;
    }
    return false;
}

void CharacterViewerWindow::loadActor(int actorIndex)
{
    m_scene->setSelectedActor(actorIndex);
    updateLists();
}

void CharacterViewerWindow::loadSkin(int skinID)
{
    m_scene->setSelectedSkinID(skinID);
    updateLists();
}

void CharacterViewerWindow::loadAnimation(QString animName)
{
    m_scene->setSelectedAnimName(animName);
    updateLists();
}

void CharacterViewerWindow::copyAnimations()
{
    CharacterModel *charModel = m_scene->actor()->model();
    if(!charModel)
        return;
    Skeleton *charSkel = charModel->skeleton();
    if(!charSkel)
        return;
    QDialog d;
    d.setWindowTitle("Select a character to copy animations from");
    QComboBox *charList = new QComboBox();
    foreach(CharacterPack *pack, m_scene->game()->packs()->characterPacks())
    {
        const QMap<QString, CharacterModel *> &actors = pack->models();
        foreach(QString charName, actors.keys())
        {
            Skeleton *skel = actors.value(charName)->skeleton();
            if(skel)
                charList->addItem(charName);
        }
    }
    
    QDialogButtonBox *buttons = new QDialogButtonBox();
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, SIGNAL(accepted()), &d, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), &d, SLOT(reject()));

    QVBoxLayout *vb = new QVBoxLayout(&d);
    vb->addWidget(charList);
    vb->addWidget(buttons);

    if(d.exec() == QDialog::Accepted)
        copyAnimations(charSkel, charList->currentText());
}

void CharacterViewerWindow::copyAnimations(Skeleton *toSkel, QString fromChar)
{
    CharacterModel *model = m_scene->game()->packs()->findCharacter(fromChar);
    if(model)
    {
        Skeleton *fromSkel = model->skeleton();
        if(fromSkel)
        {
            toSkel->copyAnimationsFrom(fromSkel);
            updateLists();
        }
    }
}

void CharacterViewerWindow::updateLists()
{
    // Update the actor list.
    const QVector<ActorInfo> &actors = m_scene->actors();
    m_actorText->clear();
    foreach(ActorInfo actorInfo, actors)
    {
        m_actorText->addItem(actorInfo.displayName);
    }
    m_actorText->setEnabled(actors.count() > 1);
    if((m_scene->selectedActor() < 0) && (actors.count() > 0))
    {
        m_scene->setSelectedActor(0);
    }
    m_actorText->setCurrentIndex(m_scene->selectedActor());

    // Update the skin and animation lists.
    m_paletteText->clear();
    m_animationText->clear();
    CharacterModel *charModel = m_scene->actor()->model();
    if(charModel)
    {
        Skeleton *skel = charModel->skeleton();
        if(skel)
        {
            foreach(QString animName, skel->animations().keys())
                m_animationText->addItem(animName);
        }
        for(unsigned i = 0; i < charModel->skinCount(); i++)
        {
            m_paletteText->addItem(QString("%1").arg(i));
        }
        m_paletteText->setCurrentIndex(m_scene->actor()->skinID());
        m_animationText->setCurrentIndex(m_animationText->findText(m_scene->selectedAnimName()));
    }
    m_animationText->setEnabled(m_animationText->count() > 1);
    m_paletteText->setEnabled(m_paletteText->count() > 1);
}

void CharacterViewerWindow::updateMenus()
{
    switch(m_scene->skinningMode())
    {
    default:
    case CharacterScene::SoftwareSkinning:
        m_softwareSkinningAction->setChecked(true);
        break;
    case CharacterScene::HardwareSkinning:
        m_hardwareSkinningAction->setChecked(true);
        break;
    }
  //  m_showFpsAction->setChecked(m_viewport->showStats());
}

void CharacterViewerWindow::clear()
{
    m_viewport->makeCurrent();
    m_scene->clear();
}

void CharacterViewerWindow::setSoftwareSkinning()
{
    m_scene->setSkinningMode(CharacterScene::SoftwareSkinning);
    updateMenus();
}

void CharacterViewerWindow::setHardwareSkinning()
{
    m_scene->setSkinningMode(CharacterScene::HardwareSkinning);
    updateMenus();
}
