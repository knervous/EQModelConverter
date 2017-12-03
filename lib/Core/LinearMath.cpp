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

#include <cmath>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include "EQuilibre/Core/LinearMath.h"

using namespace std;

bool fequal(double a, double b)
{
    return fabs(a - b) < 1e-16;
}

vec3 vec3::normalized() const
{
    float w = (float)sqrt(lengthSquared());
    return vec3(x / w, y / w, z / w);
}

float vec3::lengthSquared() const
{
    return x * x + y * y + z * z;
}

float vec3::dot(const vec3 &a, const vec3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3::cross(const vec3 &u, const vec3 &v)
{
    vec3 n;
    n.x = (u.y * v.z - u.z * v.y);
    n.y = (u.z * v.x - u.x * v.z);
    n.z = (u.x * v.y - u.y * v.x);
    return n;
}

vec3 vec3::normal(const vec3 &a, const vec3 &b, const vec3 &c)
{
    // the cross-product of AB and AC is the normal of ABC
    return cross(b - a, c - a).normalized();
}

vec3 operator-(const vec3 &a)
{
    return vec3(-a.x, -a.y, -a.z);
}

vec3 operator+(const vec3 &a, const vec3 &b)
{
    vec3 u;
    u.x = a.x + b.x;
    u.y = a.y + b.y;
    u.z = a.z + b.z;
    return u;
}

vec3 operator-(const vec3 &a, const vec3 &b)
{
    vec3 u;
    u.x = a.x - b.x;
    u.y = a.y - b.y;
    u.z = a.z - b.z;
    return u;
}

vec3 operator*(const vec3 &a, float scalar)
{
    vec3 u;
    u.x = a.x * scalar;
    u.y = a.y * scalar;
    u.z = a.z * scalar;
    return u;
}

vec4 operator-(const vec4 &a)
{
    return vec4(-a.x, -a.y, -a.z, -a.w);
}

vec4 operator+(const vec4 &a, const vec4 &b)
{
    vec4 u;
    u.x = a.x + b.x;
    u.y = a.y + b.y;
    u.z = a.z + b.z;
    u.w = a.w + b.w;
    return u;
}

vec4 operator-(const vec4 &a, const vec4 &b)
{
    vec4 u;
    u.x = a.x - b.x;
    u.y = a.y - b.y;
    u.z = a.z - b.z;
    u.w = a.w - b.w;
    return u;
}

vec4 operator*(const vec4 &a, float scalar)
{
    vec4 u;
    u.x = a.x * scalar;
    u.y = a.y * scalar;
    u.z = a.z * scalar;
    u.w = a.w * scalar;
    return u;
}

float vec4::dot(const vec4 &a, const vec4 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static QQuaternion qQuat(const vec4 &q)
{
    return QQuaternion(q.w, q.x, q.y, q.z);
}

static vec4 vQuat(const QQuaternion &q)
{
    return vec4(q.x(), q.y(), q.z(), q.scalar());
}

vec3 vec4::rotatedVec(vec3 v) const
{
    QQuaternion q = qQuat(*this);
    QVector3D res = q.rotatedVector(QVector3D(v.x, v.y, v.z));
    return vec3(res.x(), res.y(), res.z());
}

vec4 vec4::multiply(const vec4 &qa, const vec4 &qb)
{
    return vQuat(qQuat(qa) * qQuat(qb));
}

vec4 vec4::slerp(const vec4 &qa, const vec4 &qb, float f)
{
    return vQuat(QQuaternion::slerp(qQuat(qa), qQuat(qb), f));
}

vec4 vec4::quatFromEuler(const vec3 &euler)
{
    QQuaternion q = QQuaternion::fromAxisAndAngle(1.0, 0.0, 0.0, euler.x)
        * QQuaternion::fromAxisAndAngle(0.0, 1.0, 0.0, euler.y)
        * QQuaternion::fromAxisAndAngle(0.0, 0.0, 1.0, euler.z);
    return vQuat(q);
}

////////////////////////////////////////////////////////////////////////////////

matrix4::matrix4()
{
    clear();
}

matrix4::matrix4(const QMatrix4x4 &m)
{
    const qreal *md = (qreal*) m.constData();
    for(int i = 0; i < 4; i++, md += 4)
        c[i] = vec4(md[0], md[1], md[2], md[3]);
}

QMatrix4x4 matrix4::toQMatrix() const
{
    return QMatrix4x4(c[0].x, c[1].x, c[2].x, c[3].x,
                      c[0].y, c[1].y, c[2].y, c[3].y,
                      c[0].z, c[1].z, c[2].z, c[3].z,
                      c[0].w, c[1].w, c[2].w, c[3].w);
}

const vec4 *matrix4::columns() const
{
    return c;
}

vec3 matrix4::map(const vec3 &v) const
{
    vec4 v4(v.x, v.y, v.z, 1.0);
    vec4 r0(c[0].x, c[1].x, c[2].x, c[3].x);
    vec4 r1(c[0].y, c[1].y, c[2].y, c[3].y);
    vec4 r2(c[0].z, c[1].z, c[2].z, c[3].z);
    vec4 r3(c[0].w, c[1].w, c[2].w, c[3].w);
    float x = vec4::dot(r0, v4);
    float y = vec4::dot(r1, v4);
    float z = vec4::dot(r2, v4);
    float w = vec4::dot(r3, v4);
    return vec3(x / w, y / w, z / w);
}


void matrix4::clear()
{
    for(int i = 0; i < 4; i++)
        c[i] = vec4();
}

void matrix4::setIdentity()
{
    c[0] = vec4(1.0, 0.0, 0.0, 0.0);
    c[1] = vec4(0.0, 1.0, 0.0, 0.0);
    c[2] = vec4(0.0, 0.0, 1.0, 0.0);
    c[3] = vec4(0.0, 0.0, 0.0, 1.0);
}

void matrix4::transpose()
{
    c[0] = vec4(c[0].x, c[1].x, c[2].x, c[3].x);
    c[1] = vec4(c[0].y, c[1].y, c[2].y, c[3].y);
    c[2] = vec4(c[0].z, c[1].z, c[2].z, c[3].z);
    c[3] = vec4(c[0].w, c[1].w, c[2].w, c[3].w);
}

matrix4 matrix4::translate(float dx, float dy, float dz)
{
    matrix4 m;
    m.setIdentity();
    m.c[3] = vec4(dx, dy, dz, 1.0);
    return m;
}

matrix4 matrix4::translate(const vec3 &d)
{
    matrix4 m;
    m.setIdentity();
    m.c[3] = vec4(d.x, d.y, d.z, 1.0);
    return m;
}

matrix4 matrix4::rotate(float angle, float x, float y, float z)
{
    float theta = angle / 180.0 * 3.141562;
    float c = cos(theta);
    float s = sin(theta);
    float t = 1 - c;
    matrix4 m;
    m.c[0] = vec4(t * x * x + c, t * x * y + s * z, t * x * z - s * y, 0.0);
    m.c[1] = vec4(t * x * y - s * z, t * y * y + c, t * y * z + s * x, 0.0);
    m.c[2] = vec4(t * x * z + s * y, t * y * z - s * x, t * z * z + c, 0.0);
    m.c[3] = vec4(0.0, 0.0, 0.0, 1.0);
    return m;
}

matrix4 matrix4::scale(float sx, float sy, float sz)
{
    matrix4 m;
    m.c[0] = vec4(sx, 0.0, 0.0, 0.0);
    m.c[1] = vec4(0.0, sy, 0.0, 0.0);
    m.c[2] = vec4(0.0, 0.0, sz, 0.0);
    m.c[3] = vec4(0.0, 0.0, 0.0, 1.0);
    return m;
}

matrix4 matrix4::rotate(const vec4 &q)
{
    QMatrix4x4 m;
    m.setToIdentity();
    m.rotate(qQuat(q));
    matrix4 m2(m);
    return m2;
}

// The following three functions have been adapted from Qt:
// http://qt.gitorious.org/qt/qt/blobs/raw/4.7/src/gui/math3d/qmatrix4x4.h
// http://qt.gitorious.org/qt/qt/blobs/raw/4.7/src/gui/math3d/qmatrix4x4.cpp

matrix4 matrix4::perspective(float angle, float aspect, float nearPlane, float farPlane)
{
    matrix4 m;
    if (nearPlane == farPlane || aspect == 0.0)
        return m;
    float radians = (angle / 2.0) * 3.141562 / 180.0;
    float sine = sin(radians);
    if(fequal(sine, 0.0))
        return m;
    float cotan = cos(radians) / sine;
    float clip = farPlane - nearPlane;
    m.c[0] = vec4(cotan / aspect, 0.0, 0.0, 0.0);
    m.c[1] = vec4(0.0, cotan, 0.0, 0.0);
    m.c[2] = vec4(0.0, 0.0, -(nearPlane + farPlane) / clip, -1.0);
    m.c[3] = vec4(0.0, 0.0, -(2.0 * nearPlane * farPlane) / clip, 0.0);
    return m;
}

matrix4 matrix4::ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    matrix4 m;
    if (left == right || bottom == top || nearPlane == farPlane)
        return m;
    float width = right - left;
    float invheight = top - bottom;
    float clip = farPlane - nearPlane;
    m.c[0] = vec4(2.0 / width, 0.0, 0.0, 0.0);
    m.c[1] = vec4(0.0, 2.0 / invheight, 0.0, 0.0);
    m.c[2] = vec4(0.0, 0.0, -2.0 / clip, 0.0);
    m.c[3] = vec4(-(left + right) / width, -(top + bottom) / invheight, -(nearPlane + farPlane) / clip, 1.0);
    return m;
}

matrix4 matrix4::operator*(const matrix4 &b) const
{
    matrix4 m;
    m.c[0].x = c[0].x * b.c[0].x + c[1].x * b.c[0].y + c[2].x * b.c[0].z + c[3].x * b.c[0].w;
    m.c[0].y = c[0].y * b.c[0].x + c[1].y * b.c[0].y + c[2].y * b.c[0].z + c[3].y * b.c[0].w;
    m.c[0].z = c[0].z * b.c[0].x + c[1].z * b.c[0].y + c[2].z * b.c[0].z + c[3].z * b.c[0].w;
    m.c[0].w = c[0].w * b.c[0].x + c[1].w * b.c[0].y + c[2].w * b.c[0].z + c[3].w * b.c[0].w;
    
    m.c[1].x = c[0].x * b.c[1].x + c[1].x * b.c[1].y + c[2].x * b.c[1].z + c[3].x * b.c[1].w;
    m.c[1].y = c[0].y * b.c[1].x + c[1].y * b.c[1].y + c[2].y * b.c[1].z + c[3].y * b.c[1].w;
    m.c[1].z = c[0].z * b.c[1].x + c[1].z * b.c[1].y + c[2].z * b.c[1].z + c[3].z * b.c[1].w;
    m.c[1].w = c[0].w * b.c[1].x + c[1].w * b.c[1].y + c[2].w * b.c[1].z + c[3].w * b.c[1].w;
    
    m.c[2].x = c[0].x * b.c[2].x + c[1].x * b.c[2].y + c[2].x * b.c[2].z + c[3].x * b.c[2].w;
    m.c[2].y = c[0].y * b.c[2].x + c[1].y * b.c[2].y + c[2].y * b.c[2].z + c[3].y * b.c[2].w;
    m.c[2].z = c[0].z * b.c[2].x + c[1].z * b.c[2].y + c[2].z * b.c[2].z + c[3].z * b.c[2].w;
    m.c[2].w = c[0].w * b.c[2].x + c[1].w * b.c[2].y + c[2].w * b.c[2].z + c[3].w * b.c[2].w;
    
    m.c[3].x = c[0].x * b.c[3].x + c[1].x * b.c[3].y + c[2].x * b.c[3].z + c[3].x * b.c[3].w;
    m.c[3].y = c[0].y * b.c[3].x + c[1].y * b.c[3].y + c[2].y * b.c[3].z + c[3].y * b.c[3].w;
    m.c[3].z = c[0].z * b.c[3].x + c[1].z * b.c[3].y + c[2].z * b.c[3].z + c[3].z * b.c[3].w;
    m.c[3].w = c[0].w * b.c[3].x + c[1].w * b.c[3].y + c[2].w * b.c[3].z + c[3].w * b.c[3].w;
    return m;
}

matrix4 matrix4::lookAt(vec3 eye, vec3 center, vec3 up)
{
    vec3 forward = (center - eye).normalized();
    vec3 side = vec3::cross(forward, up).normalized();
    up = vec3::cross(side, forward);

    matrix4 m;
    m.c[0] = vec4(side.x, up.x, -forward.x, 0.0);
    m.c[1] = vec4(side.y, up.y, -forward.y, 0.0);
    m.c[2] = vec4(side.z, up.z, -forward.z, 0.0);
    m.c[3] = vec4(0.0, 0.0, 0.0, 1.0);
    return m * matrix4::translate(-eye.x, -eye.y, -eye.z);
}
