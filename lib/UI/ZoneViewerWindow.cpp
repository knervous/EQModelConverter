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
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include "EQuilibre/UI/ZoneViewerWindow.h"

#include "EQuilibre/UI/ZoneScene.h"
#include "EQuilibre/UI/SceneViewport.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/UI/Scene.h"
#include "EQuilibre/Core/Character.h"
#include "EQuilibre/Core/Skeleton.h"
#include "EQuilibre/Core/SoundTrigger.h"
#include "EQuilibre/Game/Game.h"
//#include "EQuilibre/Game/GameClient.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/Zone.h"

ZoneViewerWindow::ZoneViewerWindow(Game *game, QWidget *parent) : QMainWindow(parent)
{
    m_game = game;
    m_scene = new ZoneScene(game);
    m_viewport = new SceneViewport(m_scene);
    setWindowTitle(isClient() ? "EQuilibre" : "EQuilibre Zone Viewer");
    setCentralWidget(m_viewport);
    initMenus();
    connect(m_scene, SIGNAL(loading()), this, SLOT(handleZoneLoading()));
    connect(m_scene, SIGNAL(loaded()), this, SLOT(handleZoneLoaded()));
    connect(m_scene, SIGNAL(unloaded()), this, SLOT(handleZoneUnloaded()));
}

ZoneViewerWindow::~ZoneViewerWindow()
{
    m_game->clear();
    delete m_viewport;
    delete m_scene;
}

void ZoneViewerWindow::initMenus()
{
    QMenu *fileMenu = new QMenu(this);
    fileMenu->setTitle("&File");

    QAction *openAction = new QAction("&Open S3D archive...", this);
    openAction->setShortcut(QKeySequence::Open);
    QAction *clearAction = new QAction("&Clear", this);
    QAction *selectDirAction = new QAction("Select Asset Directory...", this);
    
    QAction *gotoZoneAction = new QAction("&Go to...", this);
    gotoZoneAction->setShortcut(QKeySequence::Find);

    QAction *quitAction = new QAction("&Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);

    if(!isClient())
    {
        fileMenu->addAction(openAction);
        fileMenu->addAction(clearAction);
        fileMenu->addAction(selectDirAction);   
    }
    fileMenu->addAction(gotoZoneAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QMenu *renderMenu = new QMenu(this);
    renderMenu->setTitle("&Render");

    m_noLightingAction = new QAction("No Lighting", this);
    m_bakedLightingAction = new QAction("Baked Lighting", this);
    m_debugVertexColorAction = new QAction("Show Vertex Color", this);
    m_debugTextureFactorAction = new QAction("Show Texture Blend Factor", this);
    m_debugDiffuseAction = new QAction("Show Diffuse Factor", this);
    m_noLightingAction->setCheckable(true);
    m_bakedLightingAction->setCheckable(true);
    m_debugVertexColorAction->setCheckable(true);
    m_debugTextureFactorAction->setCheckable(true);
    m_debugDiffuseAction->setCheckable(true);
    m_bakedLightingAction->setEnabled(false);
    m_debugDiffuseAction->setEnabled(false);
    QActionGroup *lightingActions = new QActionGroup(this);
    lightingActions->addAction(m_noLightingAction);
    lightingActions->addAction(m_bakedLightingAction);
    lightingActions->addAction(m_debugVertexColorAction);
    lightingActions->addAction(m_debugTextureFactorAction);
    lightingActions->addAction(m_debugDiffuseAction);

    m_showFpsAction = new QAction("Show Stats", this);
    m_showFpsAction->setCheckable(true);
    m_showZoneAction = createGameFlagAction("Show Terrain", eGameShowTerrain);
    m_showZoneObjectsAction = createGameFlagAction("Show Objects", eGameShowObjects);
    m_showZoneActorsAction = createGameFlagAction("Show Characters", eGameShowCharacters);
    m_showSkyAction = createGameFlagAction("Show Sky", eGameShowSky);
    m_showFogAction = createGameFlagAction("Show Fog", eGameShowFog);
    m_showZoneObjectsAction->setChecked(m_game->hasFlag(eGameShowObjects));
    m_cullZoneObjectsAction = createGameFlagAction("Frustum Culling of Objects", eGameCullObjects);
    m_showSoundTriggersAction = createGameFlagAction("Show Sound Triggers", eGameShowFog);
    m_gpuSkinningAction = createGameFlagAction("GPU skinning", eGameGPUSkinning);

    renderMenu->addAction(m_noLightingAction);
    renderMenu->addAction(m_bakedLightingAction);
    renderMenu->addAction(m_debugVertexColorAction);
    renderMenu->addAction(m_debugTextureFactorAction);
    renderMenu->addAction(m_debugDiffuseAction);
    renderMenu->addSeparator();
    renderMenu->addAction(m_showFpsAction);
    renderMenu->addAction(m_showZoneAction);
    renderMenu->addAction(m_showZoneObjectsAction);
    renderMenu->addAction(m_showZoneActorsAction);
    renderMenu->addAction(m_showSkyAction);
    renderMenu->addAction(m_cullZoneObjectsAction);
    renderMenu->addAction(m_showFogAction);
    renderMenu->addAction(m_showSoundTriggersAction);
    renderMenu->addAction(m_gpuSkinningAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(renderMenu);

    updateMenus();

    connect(openAction, SIGNAL(triggered()), this, SLOT(openArchive()));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clearZone()));
    connect(selectDirAction, SIGNAL(triggered()), this, SLOT(selectAssetDir()));
    connect(gotoZoneAction, SIGNAL(triggered()), this, SLOT(gotoZone()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(quit()));

    connect(m_noLightingAction, SIGNAL(triggered()), this, SLOT(setNoLighting()));
    connect(m_bakedLightingAction, SIGNAL(triggered()), this, SLOT(setBakedLighting()));
    connect(m_debugVertexColorAction, SIGNAL(triggered()), this, SLOT(setDebugVertexColor()));
    connect(m_debugTextureFactorAction, SIGNAL(triggered()), this, SLOT(setDebugTextureFactor()));
    connect(m_debugDiffuseAction, SIGNAL(triggered()), this, SLOT(setDebugDiffuse()));
   // connect(m_showFpsAction, SIGNAL(toggled(bool)), m_viewport, SLOT(setShowStats(bool)));
    connect(m_showZoneAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZone(bool)));
    connect(m_showZoneObjectsAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZoneObjects(bool)));
    connect(m_showZoneActorsAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZoneActors(bool)));
    connect(m_showSkyAction, SIGNAL(toggled(bool)), m_scene, SLOT(showSky(bool)));
    connect(m_showFogAction, SIGNAL(toggled(bool)), m_scene, SLOT(showFog(bool)));
    connect(m_cullZoneObjectsAction, SIGNAL(toggled(bool)), m_scene, SLOT(setFrustumCulling(bool)));
    connect(m_showSoundTriggersAction, SIGNAL(toggled(bool)), m_scene, SLOT(showSoundTriggers(bool)));
    connect(m_gpuSkinningAction, SIGNAL(toggled(bool)), m_scene, SLOT(enableGPUSkinning(bool)));
}

QAction * ZoneViewerWindow::createGameFlagAction(QString text, GameFlags flag)
{
    QAction *a = new QAction(text, this);
    a->setCheckable(true);
    a->setChecked(m_game->hasFlag(flag));
    return a;
}

bool ZoneViewerWindow::isClient() const
{
    return m_game->client() != NULL;
}

void ZoneViewerWindow::openArchive()
{
    QString filePath;
    // OpenGL rendering interferes wtih QFileDialog
    bool animate = m_viewport->animation();
    m_viewport->setAnimation(false);
    filePath = QFileDialog::getOpenFileName(0, "Select a S3D file to open",
        m_scene->game()->assetPath(), "S3D Archive (*.s3d)");
    m_viewport->setAnimation(animate);
    if(!filePath.isEmpty())
    {
        QFileInfo info(filePath);
        m_viewport->makeCurrent();
        m_scene->loadZone(info.absolutePath(), info.baseName());
    }
}

void ZoneViewerWindow::clearZone()
{
    m_game->zone()->unload();
}

void ZoneViewerWindow::closeEvent(QCloseEvent *e)
{
//    GameClient *client = m_game->client();
//    if(client)
//    {
//        // Notify the server that we are disconnecting and wait for the reply.
//        //client->logout();
//        e->ignore();
//    }
}

void ZoneViewerWindow::quit()
{
//    GameClient *client = m_game->client();
//    if(client)
//        client->logout();
//    else
//        close();
}

void ZoneViewerWindow::handleZoneLoading()
{
    m_viewport->setAnimation(true);
    m_viewport->setFocus();
    if(isClient())
    {
        this->show();
    }
}

void ZoneViewerWindow::handleZoneLoaded()
{
    
}

void ZoneViewerWindow::handleZoneUnloaded()
{
//    GameClient *client = m_game->client();
//    m_viewport->setAnimation(false);
//    if(isClient() && !client->isChangingZones())
//    {
//        this->hide();
//    }
}

void ZoneViewerWindow::selectAssetDir()
{
    // OpenGL rendering interferes wtih QFileDialog
//    bool animate = m_viewport->animation();
//    m_viewport->setAnimation(false);
//    LauncherWindow::selectAssetDirectory(m_game);
//    m_viewport->setAnimation(animate);
}

void ZoneViewerWindow::updateMenus()
{
    switch(m_scene->lightingMode())
    {
    default:
    case RenderProgram::NoLighting:
        m_noLightingAction->setChecked(true);
        break;
    case RenderProgram::BakedLighting:
        m_bakedLightingAction->setChecked(true);
        break;
    case RenderProgram::DebugVertexColor:
        m_debugVertexColorAction->setChecked(true);
        break;
    case RenderProgram::DebugTextureFactor:
        m_debugTextureFactorAction->setChecked(true);
        break;
    case RenderProgram::DebugDiffuse:
        m_debugDiffuseAction->setChecked(true);
        break;
    }

  //  m_showFpsAction->setChecked(m_viewport->showStats());
}

//void ZoneViewerWindow::gotoZone()
//{
//    GotoZoneDialog d(m_game, this);
//    d.setDestPos(vec3(0.0f, 0.0f, 0.0f));
//    d.setDestZoneID(m_game->zone()->id());
//    if(d.exec() == QDialog::Accepted)
//    {
//       // m_scene->gotoZone(d.destPos(), d.destZoneID());
//    }
//}

void ZoneViewerWindow::setNoLighting()
{
    m_scene->setLightingMode(RenderProgram::NoLighting);
    updateMenus();
}

void ZoneViewerWindow::setBakedLighting()
{
    m_scene->setLightingMode(RenderProgram::BakedLighting);
    updateMenus();
}

void ZoneViewerWindow::setDebugVertexColor()
{
    m_scene->setLightingMode(RenderProgram::DebugVertexColor);
    updateMenus();
}

void ZoneViewerWindow::setDebugTextureFactor()
{
    m_scene->setLightingMode(RenderProgram::DebugTextureFactor);
    updateMenus();
}

void ZoneViewerWindow::setDebugDiffuse()
{
    m_scene->setLightingMode(RenderProgram::DebugDiffuse);
    updateMenus();
}

////////////////////////////////////////////////////////////////////////////////

GotoZoneDialog::GotoZoneDialog(Game *game, QWidget *parent) : QDialog(parent)
{
    QDoubleValidator *coordValid = new QDoubleValidator(-9999.0, 9999.0, 2, this);
    
    QLabel *helpLabel = new QLabel("Enter the destination:");
    
    m_game = game;
    m_destZoneText = new QComboBox();
    m_destZoneText->setModel(game->zones());
    m_destZoneText->setModelColumn(3);
    
    m_coordX = new QLineEdit("0.0");
    m_coordX->setValidator(coordValid);
    m_coordY = new QLineEdit("0.0");
    m_coordY->setValidator(coordValid);
    m_coordZ = new QLineEdit("0.0");
    m_coordZ->setValidator(coordValid);
    
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    
    setWindowTitle("Go to");
    
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_coordX);
    hbox->addWidget(m_coordY);
    hbox->addWidget(m_coordZ);
    
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->addWidget(m_destZoneText);
    vbox->addWidget(helpLabel);
    vbox->addLayout(hbox);
    vbox->addWidget(m_buttons);
    
    connect(m_buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

uint16_t GotoZoneDialog::destZoneID() const
{
    const QVector<ZoneInfo> &zones = m_game->zones()->items();
    const ZoneInfo &info = zones.value(m_destZoneText->currentIndex());
    return info.zoneID;
}

void GotoZoneDialog::setDestZoneID(uint16_t zoneID)
{
    const QVector<ZoneInfo> &zones = m_game->zones()->items();
    for(unsigned i = 0; i < zones.size(); i++)
    {
        ZoneInfo zone = zones.value(i);
        if(zone.zoneID == zoneID)
        {
            m_destZoneText->setCurrentIndex(i);
            break;
        }
    }
}

vec3 GotoZoneDialog::destPos() const
{
    vec3 newPos;
    newPos.x = m_coordX->text().toFloat();
    newPos.y = m_coordY->text().toFloat();
    newPos.z = m_coordZ->text().toFloat();
    return newPos;
}

void GotoZoneDialog::setDestPos(const vec3 &newPos)
{
    m_coordX->setText(QString("%1").arg(newPos.x));
    m_coordY->setText(QString("%1").arg(newPos.y));
    m_coordZ->setText(QString("%1").arg(newPos.z));
}
