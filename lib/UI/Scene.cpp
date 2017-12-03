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

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include "EQuilibre/UI/Scene.h"
#include "EQuilibre/Render/RenderContext.h"

Scene::Scene(RenderContext *renderCtx)
{
    m_renderCtx = renderCtx;
}

Scene::~Scene()
{
}

RenderContext * Scene::renderContext() const
{
    return m_renderCtx;
}

const QVector<TextElement> & Scene::textElements() const
{
    return m_textElements;
}

void Scene::init()
{
}

void Scene::update()
{
}

void Scene::keyPressEvent(QKeyEvent *e)
{
    e->ignore();
}

void Scene::keyReleaseEvent(QKeyEvent *e)
{
    e->ignore();
}

void Scene::mouseMoveEvent(QMouseEvent *e)
{
    e->ignore();
}

void Scene::mousePressEvent(QMouseEvent *e)
{
    e->ignore();
}

void Scene::mouseReleaseEvent(QMouseEvent *e)
{
    e->ignore();
}

void Scene::wheelEvent(QWheelEvent *e)
{
    e->ignore();
}
