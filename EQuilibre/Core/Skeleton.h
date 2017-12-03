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

#ifndef EQUILIBRE_CORE_SKELETON_H
#define EQUILIBRE_CORE_SKELETON_H

#include <QObject>
#include <QMap>
#include <QVector>
#include "EQuilibre/Core/Platform.h"
#include "EQuilibre/Core/LinearMath.h"
#include "EQuilibre/Core/WLDData.h"

class TrackFragment;
class MeshFragment;
class Animation;

class SkeletonNode
{
public:
    WLDFragmentRef name;
    uint32_t flags;
    TrackFragment *track;
    MeshFragment *mesh;
    QVector<uint32_t> children;
};

typedef QVector<SkeletonNode> SkeletonTree;

class BoneTransform
{
public:
    vec3 location;
    float padding;
    vec4 rotation;

    void clear();
    vec3 map(const vec3 &v) const;
    vec4 map(const vec4 &v) const;
    BoneTransform map(const BoneTransform &t) const;
    void toDualQuaternion(vec4 &d0, vec4 &d1) const;

    static BoneTransform identity();
    static BoneTransform interpolate(const BoneTransform &a,
                                     const BoneTransform &b,
                                     float c);
};

typedef std::vector<BoneTransform> BoneSet;

class BoneTrack
{
public:
    QString name;
    const BoneTransform *frames;
    uint32_t frameCount;
    
    BoneTransform interpolate(double f) const;
};

typedef std::vector<BoneTrack> BoneTrackSet;

/*!
  \brief Holds information about a model's skeleton, used for animation.
  */
class Skeleton : public QObject
{
public:
    Skeleton(SkeletonTree tree, const BoneTrackSet &tracks, float boundingRadius,
             QObject *parent = 0);

    Animation *pose() const;
    const QMap<QString, Animation *> & animations() const;
    const SkeletonTree & tree() const;
    float boundingRadius() const;
    void setBoundingRadius(float newRadius);

    void addTrack(QString animName, const BoneTrack &track);
    void copyAnimationsFrom(Skeleton *skel);
    Animation * copyFrom(Skeleton *skel, QString animName);

private:
    SkeletonTree m_tree;
    QMap<QString, Animation *> m_animations;
    Animation *m_pose;
    float m_boundingRadius;
};

/*!
  \brief Describes one way of animating a model's skeleton.
  */
class Animation : public QObject
{
public:
    Animation(QString name, const BoneTrackSet &tracks, Skeleton *skel,
              QObject *parent = 0);

    QString name() const;
    const BoneTrackSet & tracks() const;
    Skeleton * skeleton() const;
    double duration() const;
    uint32_t frameCount() const;
    double timeAtFrame(int f) const;
    double frameAtTime(double t) const;

    int findTrack(QString name) const;
    void replaceTrack(const BoneTrack &newTrack);
    Animation * copy(QString newName, QObject *parent = 0) const;
    void transformationsAtTime(BoneSet &bones, double t) const;
    void transformationsAtFrame(BoneSet &bones, double f) const;
    void transformAll(BoneTransform *animData, uint32_t maxFrames) const;

private:
    void transformPiece(BoneSet &bones,
                        const SkeletonTree &tree,
                        uint32_t pieceID, double f,
                        BoneTransform &parentTrans) const;
    void transformPiece(BoneTransform *animData,
                        const SkeletonTree &tree,
                        uint32_t pieceID, uint32_t maxFrames, 
                        const BoneTransform *parentData) const;

    QString m_name;
    BoneTrackSet m_tracks;
    Skeleton *m_skel;
    uint32_t m_frameCount;
};

class AnimationArray
{
public:
    AnimationArray();
    ~AnimationArray();

    BoneTransform *data() const;
    uint32_t dataSize() const;
    
    uint32_t maxFrames() const;
    uint32_t maxTracks() const;
    uint32_t maxAnims() const;
    vec3 textureDim() const;
    
    Animation * animation(uint32_t animID) const;

    bool load(Animation **animations, size_t count);
    void transformationsAtFrame(uint32_t animID, double f, BoneSet &bones);
    void transformationsAtTime(uint32_t animID, double t, BoneSet &bones);
    void transformationAtFrame(uint32_t animID, uint32_t trackID, double f, BoneTransform &bone);
    void transformationAtTime(uint32_t animID, uint32_t trackID, double t, BoneTransform &bone);
    void clear();

private:
    void updateTextureDimensions();

    BoneTransform *m_data;
    std::vector<Animation *> m_animations;
    uint32_t m_maxFrames;
    uint32_t m_maxTracks;
    uint32_t m_maxAnims;
};

#endif
