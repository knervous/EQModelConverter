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

#include <cmath>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include "EQuilibre/UI/SceneViewport.h"
#include "EQuilibre/UI/Scene.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Material.h"
#include "EQuilibre/Render/RenderContext.h"


SceneViewport::SceneViewport(Scene *scene, QWidget *parent) : QGLWidget(parent)
{
    setMinimumSize(640, 480);
    m_scene = scene;
    m_renderCtx = scene->renderContext();
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(0);
#ifdef USE_VTUNE_PROFILER
    m_traceDomain = __itt_domain_create("SceneViewport");
#endif
    setAutoFillBackground(true);
    connect(m_renderTimer, SIGNAL(timeout()), this, SLOT(update()));
}

SceneViewport::~SceneViewport()
{
}

void SceneViewport::initializeGL()
{
    if(m_renderCtx->init())
    {
        m_scene->init();
        emit initialized();
    }
}

void SceneViewport::resizeGL(int w, int h)
{
    if(m_renderCtx->isValid())
        m_renderCtx->setupViewport(w, h);
}

void SceneViewport::paintEvent(QPaintEvent *)
{
    if(!m_renderCtx->isValid())
        return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
#ifdef USE_VTUNE_PROFILER
    __itt_frame_begin_v3(m_traceDomain, NULL); 
#endif
    m_scene->update();
    m_scene->draw();
#ifdef USE_VTUNE_PROFILER
    __itt_frame_end_v3(m_traceDomain, NULL); 
#endif
    paintOverlay(painter); // XXX fix QPainter overlay artifacts on Windows.
}

void SceneViewport::paintOverlay(QPainter &painter)
{
    paintTextElements(&painter);
}

bool SceneViewport::animation() const
{
    return m_animate;
}

void SceneViewport::setAnimation(bool enabled)
{
    m_animate = enabled;
    updateAnimationState();
}


void SceneViewport::paintTextElements(QPainter *p)
{
    foreach(const TextElement &e, m_scene->textElements())
    {
        p->setFont(e.font);
        p->setPen(QPen(e.color));
        p->drawText(e.rect, e.text);
    }
}

void SceneViewport::updateAnimationState()
{
    if(m_animate)
    {
        m_renderTimer->start();
    }
    else
    {
        m_renderTimer->stop();
    }
}

void SceneViewport::keyPressEvent(QKeyEvent *e)
{
    m_scene->keyPressEvent(e);
    //if(!e->isAccepted())
    {
        QGLWidget::keyPressEvent(e);
        //return;
    }
    update();
}

void SceneViewport::keyReleaseEvent(QKeyEvent *e)
{
    m_scene->keyReleaseEvent(e);
    //if(!e->isAccepted())
    {
        QGLWidget::keyReleaseEvent(e);
        //return;
    }
    update();
}

void SceneViewport::mouseMoveEvent(QMouseEvent *e)
{
    m_scene->mouseMoveEvent(e);
    //if(!e->isAccepted())
    {
        QGLWidget::mouseMoveEvent(e);
        //return;
    }
    update();
}

void SceneViewport::mousePressEvent(QMouseEvent *e)
{
    m_scene->mousePressEvent(e);
    if(e->button() & Qt::LeftButton)
        setFocus();
    //if(!e->isAccepted())
    {
        QGLWidget::mousePressEvent(e);
        //return;
    }
    update();
}

void SceneViewport::mouseReleaseEvent(QMouseEvent *e)
{
    m_scene->mouseReleaseEvent(e);
    //if(!e->isAccepted())
    {
        QGLWidget::mouseReleaseEvent(e);
        //return;
    }
    update();
}

void SceneViewport::wheelEvent(QWheelEvent *e)
{
    m_scene->wheelEvent(e);
    //if(!e->isAccepted())
    {
        QGLWidget::wheelEvent(e);
        //return;
    }
    update();
}
