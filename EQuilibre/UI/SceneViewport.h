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

#ifndef EQUILIBRE_UI_SCENE_VIEWPORT_H
#define EQUILIBRE_UI_SCENE_VIEWPORT_H

#include <QGLWidget>
#include <QTime>
#include <QVector>
#include "EQuilibre/Core/Platform.h"

#ifdef USE_VTUNE_PROFILER
#include <ittnotify.h>
#endif

class QTimer;
class QPainter;
class QGLFormat;
class Scene;
class RenderContext;

class  SceneViewport : public QGLWidget
{
    Q_OBJECT

public:
    SceneViewport(Scene *scene, QWidget *parent = 0);
    virtual ~SceneViewport();

    bool animation() const;
    void setAnimation(bool enabled);

    
signals:
    void initialized();

public slots:


protected:
    virtual void initializeGL();
    virtual void resizeGL(int width, int height);
    virtual void paintEvent(QPaintEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

private slots:


private:
    void paintOverlay(QPainter &painter);
    void paintTextElements(QPainter *p);
    void updateAnimationState();

    Scene *m_scene;
    RenderContext *m_renderCtx;
    QTimer *m_renderTimer;
    bool m_animate;
    
#ifdef USE_VTUNE_PROFILER
    __itt_domain *m_traceDomain;
#endif
};

#endif
