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

#ifndef EQUILIBRE_UI_SCENE_H
#define EQUILIBRE_UI_SCENE_H

#include <QFont>
#include <QObject>
#include <QRectF>
#include <QVector>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/LinearMath.h"

class RenderContext;
class QKeyEvent;
class QMouseEvent;
class QFont;
class QWheelEvent;

struct MouseState
{
    bool active;
    int x0;
    int y0;
    vec3 last;        // value of delta/theta when the user last clicked
};

struct TextElement
{
    QString text;
    QRectF rect;
    Qt::GlobalColor color;
    QFont font;
};

class  Scene : public QObject
{
    Q_OBJECT

public:
    Scene(RenderContext *renderCtx);
    virtual ~Scene();
    
    RenderContext *renderContext() const;
    
    const QVector<TextElement> & textElements() const;

    virtual void init();
    virtual void update();
    virtual void draw() = 0;

    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

protected:
    RenderContext *m_renderCtx;
    QString m_frameLog;
    QVector<TextElement> m_textElements;
};

#endif
