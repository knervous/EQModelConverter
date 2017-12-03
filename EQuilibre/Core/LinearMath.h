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

#ifndef EQUILIBRE_CORE_LINEAR_MATH_H
#define EQUILIBRE_CORE_LINEAR_MATH_H

#include <QMatrix4x4>
#include "EQuilibre/Core/Platform.h"

bool fequal(double a, double b);

class vec2
{
public:
    float x;
    float y;

    inline vec2()
    {
        x = y = 0.0;
    }

    inline vec2(float x, float y)
    {
        this->x = x;
        this->y = y;
    }
};

class vec3
{
public:
    float x;
    float y;
    float z;

    inline vec3()
    {
        x = y = z = 0.0;
    }

    inline vec3(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    
    inline vec3(vec2 xy, float z)
    {
        this->x = xy.x;
        this->y = xy.y;
        this->z = z;
    }

    vec3 normalized() const;
    float lengthSquared() const;

    static float dot(const vec3 &a, const vec3 &b);
    static vec3 cross(const vec3 &a, const vec3 &b);
    static vec3 normal(const vec3 &a, const vec3 &b, const vec3 &c);
};

vec3 operator-(const vec3 &a);
vec3 operator+(const vec3 &a, const vec3 &b);
vec3 operator-(const vec3 &a, const vec3 &b);
vec3 operator*(const vec3 &a, float scalar);

class vec4
{
public:
    float x;
    float y;
    float z;
    float w;

    inline vec4()
    {
        x = y = z = w = 0.0;
    }

    inline vec4(float x, float y, float z, float w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }
    
    inline explicit vec4(const vec3 &v)
    {
        this->x = v.x;
        this->y = v.y;
        this->z = v.z;
        this->w = 1.0;
    }
    
    vec3 asVec3() const
    {
        return vec3(x, y, z);
    }
    
    vec3 rotatedVec(vec3 v) const;
    
    static vec4 multiply(const vec4 &qa, const vec4 &qb);
    static vec4 slerp(const vec4 &qa, const vec4 &qb, float f);
    static vec4 quatFromEuler(const vec3 &euler);
    static float dot(const vec4 &a, const vec4 &b);
};

vec4 operator-(const vec4 &a);
vec4 operator+(const vec4 &a, const vec4 &b);
vec4 operator-(const vec4 &a, const vec4 &b);
vec4 operator*(const vec4 &a, float scalar);

class matrix4
{
public:

    matrix4();
    matrix4(const QMatrix4x4 &m);
    
    QMatrix4x4 toQMatrix() const;
    
    const vec4 * columns() const;
    vec3 map(const vec3 &v) const;
    void transpose();

    void clear();
    void setIdentity();

    static matrix4 translate(const vec3 &d);
    static matrix4 translate(float dx, float dy, float dz);
    static matrix4 rotate(float angle, float rx, float ry, float rz);
    static matrix4 rotate(const vec4 &q);
    static matrix4 scale(float sx, float sy, float sz);
    static matrix4 perspective(float angle, float aspect, float nearPlane, float farPlane);
    static matrix4 ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    static matrix4 lookAt(vec3 eye, vec3 center, vec3 up);
    
    matrix4 operator*(const matrix4 &b) const;
    
private:
    vec4 c[4];
};

#endif
