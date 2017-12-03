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
#include <algorithm>
#include "EQuilibre/Core/Skeleton.h"
#include "EQuilibre/Core/Fragments.h"

Skeleton::Skeleton(SkeletonTree tree, const BoneTrackSet &tracks, float boundingRadius,
                   QObject *parent) : QObject(parent)
{
    m_tree = tree;
    m_pose = new Animation("POS", tracks, this, this);
    m_boundingRadius = boundingRadius;
    m_animations.insert(m_pose->name(), m_pose);
}

Animation * Skeleton::pose() const
{
    return m_pose;
}

const SkeletonTree & Skeleton::tree() const
{
    return m_tree;
}

const QMap<QString, Animation *> & Skeleton::animations() const
{
    return m_animations;
}

float Skeleton::boundingRadius() const
{
    return m_boundingRadius;
}

void Skeleton::setBoundingRadius(float newRadius)
{
    m_boundingRadius = newRadius;
}

void Skeleton::addTrack(QString animName, const BoneTrack &track)
{
    Animation *anim = m_animations.value(animName);
    if(!anim)
    {
        anim = m_pose->copy(animName, this);
        m_animations.insert(animName, anim);
    }
    anim->replaceTrack(track);
}

void Skeleton::copyAnimationsFrom(Skeleton *skel)
{
    foreach(QString animName, skel->animations().keys())
    {
        if(!animations().contains(animName))
            copyFrom(skel, animName);
    }
}

Animation * Skeleton::copyFrom(Skeleton *skel, QString animName)
{
    if(!skel || skel == this)
        return 0;
    Animation *anim = skel->animations().value(animName);
    if(!anim)
        return 0;
    Animation *anim2 = m_pose->copy(animName, this);
    foreach(BoneTrack track, anim->tracks())
        anim2->replaceTrack(track);
    m_animations.insert(anim2->name(), anim2);
    return anim2;
}

////////////////////////////////////////////////////////////////////////////////

Animation::Animation(QString name, const BoneTrackSet &tracks, Skeleton *skel,
                     QObject *parent) : QObject(parent)
{
    m_name = name;
    m_tracks = tracks;
    m_skel = skel;
    m_frameCount = 0;
    foreach(BoneTrack track, tracks)
        m_frameCount = std::max(m_frameCount, (uint32_t)track.frameCount);
}

QString Animation::name() const
{
    return m_name;
}

const BoneTrackSet & Animation::tracks() const
{
    return m_tracks;
}

Skeleton * Animation::skeleton() const
{
    return m_skel;
}

uint32_t Animation::frameCount() const
{
    return m_frameCount;
}

double Animation::duration() const
{
    const double fps = 10.0;
    return m_frameCount / fps;
}

double Animation::timeAtFrame(int f) const
{
    const double fps = 10.0;
    return fmod(f, (double)m_frameCount) / fps;
}

double Animation::frameAtTime(double t) const
{
    const double fps = 10.0;
    return fmod(fmod(t, duration()) * fps, m_frameCount);
}

int Animation::findTrack(QString name) const
{
    for(int i = 0; i < m_tracks.size(); i++)
    {
        QString trackName = m_tracks[i].name;
        if(trackName.contains(name))
            return i;
    }
    return -1;
}

void Animation::replaceTrack(const BoneTrack &newTrack)
{
    // strip animation name and character name from track name
    QString trackName = newTrack.name.mid(6);
    for(BoneTrackSet::iterator I = m_tracks.begin(), E = m_tracks.end(); I != E; ++I)
    {
        // strip character name from track name
        const BoneTrack &oldTrack = *I;
        if(oldTrack.name.mid(3) == trackName)
        {
            *I = newTrack;
            m_frameCount = std::max(m_frameCount, (uint32_t)newTrack.frameCount);
            break;
        }
    }
}

Animation * Animation::copy(QString newName, QObject *parent) const
{
    return new Animation(newName, m_tracks, m_skel, parent);
}

void Animation::transformationsAtTime(BoneSet &bones, double t) const
{
    const double fps = 10.0;
    double f = frameAtTime(t);
    transformationsAtFrame(bones, f);
}

void Animation::transformationsAtFrame(BoneSet &bones, double f) const
{
    BoneTransform rootTrans = BoneTransform::identity();
    transformPiece(bones, m_skel->tree(), 0, f, rootTrans);
}

void Animation::transformAll(BoneTransform *animData, uint32_t maxFrames) const
{
    transformPiece(animData, m_skel->tree(), 0, maxFrames, NULL);
}

void Animation::transformPiece(BoneSet &bones,
                               const SkeletonTree &tree,
                               uint32_t pieceID, double f,
                               BoneTransform &parentTrans) const
{
    SkeletonNode piece = tree.value(pieceID);
    const BoneTrack &track = m_tracks[pieceID];
    BoneTransform pieceTrans = track.interpolate(f);
    BoneTransform globalTrans = parentTrans.map(pieceTrans);
    bones[pieceID] = globalTrans;
    foreach(uint32_t childID, piece.children)
        transformPiece(bones, tree, childID, f, globalTrans);
}

void Animation::transformPiece(BoneTransform *animData,
                               const SkeletonTree &tree,
                               uint32_t pieceID, uint32_t maxFrames, 
                               const BoneTransform *parentData) const
{
    // Make this track's transformations global using the parent track.
    SkeletonNode piece = tree.value(pieceID);
    const BoneTrack &track = m_tracks[pieceID];
    BoneTransform *trackData = animData + (maxFrames * pieceID);
    for(unsigned i = 0; i < maxFrames; i++)
    {
        // Clamp the frame index. The last frame is repeated as necessary.
        unsigned frameIdx = qMin(i, track.frameCount - 1);
        const BoneTransform &f = track.frames[frameIdx];
        trackData[i] = parentData ? parentData[i].map(f) : f;
    }    
    
    // Transform child tracks using this track's global data.
    const QVector<uint32_t> &children = piece.children;
    for(unsigned i = 0; i < children.size(); i++)
    {
        transformPiece(animData, tree, children[i], maxFrames, trackData);
    }
}

////////////////////////////////////////////////////////////////////////////////

AnimationArray::AnimationArray()
{
    m_data = NULL;
    m_maxFrames = m_maxFrames = m_maxAnims = 0;
}

AnimationArray::~AnimationArray()
{
    clear();
}

void AnimationArray::clear()
{
    delete [] m_data;
    m_data = NULL;
    m_maxFrames = m_maxFrames = m_maxAnims = 0;
    m_animations.clear();
}

BoneTransform * AnimationArray::data() const
{
    return m_data;
}

uint32_t AnimationArray::dataSize() const
{
    return m_maxFrames * m_maxTracks * m_maxAnims * sizeof(BoneTransform);
}

uint32_t AnimationArray::maxFrames() const
{
    return m_maxFrames;
}

uint32_t AnimationArray::maxTracks() const
{
    return m_maxTracks;
}

uint32_t AnimationArray::maxAnims() const
{
    return m_maxAnims;
}

vec3 AnimationArray::textureDim() const
{
    return vec3(m_maxFrames * 2, m_maxTracks, m_maxAnims);
}

Animation * AnimationArray::animation(uint32_t animID) const
{
    Animation *anim = NULL;
    if(animID < m_maxAnims)
    {
        // Fall back to animation zero (i.e. pose) if not found.
        anim = m_animations[animID];
        if(!anim && (m_maxAnims > 0))
        {
            anim = m_animations[0];
        }
    }
    return anim;
}

void AnimationArray::transformationAtTime(uint32_t animID, uint32_t trackID,
                                          double t, BoneTransform &bone)
{
    if((animID >= m_animations.size()) || !m_animations[animID] || !m_data)
    {
        return;
    }
    Animation *anim = m_animations[animID];
    double f = anim->frameAtTime(t);
    transformationAtFrame(animID, trackID, f, bone);
}

void AnimationArray::transformationsAtTime(uint32_t animID, double t, BoneSet &bones)
{
    if((animID >= m_animations.size()) || !m_animations[animID] || !m_data)
    {
        return;
    }
    Animation *anim = m_animations[animID];
    double f = anim->frameAtTime(t);
    transformationsAtFrame(animID, f, bones);
}

void AnimationArray::transformationAtFrame(uint32_t animID, uint32_t trackID,
                                           double f, BoneTransform &bone)
{
    if((trackID >= m_maxTracks) || !m_data)
    {
        return;
    }
    uint32_t animDataSize = m_maxFrames * m_maxTracks;
    const BoneTransform *animData = m_data + (animDataSize * animID);
    int frame1 = qMin(qMax((int)floor(f), 0), (int)m_maxFrames - 1);
    int frame2 = qMin(qMax(frame1 + 1, 0), (int)m_maxFrames - 1);
    const BoneTransform *trackData = animData + (m_maxFrames * trackID);
    bone = BoneTransform::interpolate(trackData[frame1], trackData[frame2],
                                      f - frame1);
}

void AnimationArray::transformationsAtFrame(uint32_t animID, double f, BoneSet &bones)
{
    if(!m_data)
    {
        return;
    }

    uint32_t animDataSize = m_maxFrames * m_maxTracks;
    const BoneTransform *animData = m_data + (animDataSize * animID);
    int frame1 = qMin(qMax((int)floor(f), 0), (int)m_maxFrames - 1);
    int frame2 = qMin(qMax(frame1 + 1, 0), (int)m_maxFrames - 1);
    for(unsigned i = 0; i < bones.size(); i++)
    {
        const BoneTransform *trackData = animData + (m_maxFrames * i);
        bones[i] = BoneTransform::interpolate(trackData[frame1], trackData[frame2],
                                              f - frame1);
    }
}

void AnimationArray::updateTextureDimensions()
{
    // The animation texture is laid out as follows:
    // * As x increases, the frame number increases by the same amount.
    //   This supposes one 'pixel' contains a bone transformation which includes
    //   translation and rotation, stored in this order.
    //   RGBA32F textures need two pixels to store one bone transformation but
    //   this will be dealt with elsewhere to keep this simple.
    // * As y increases, the track number increases by the same amount.
    // * As z increases, the animation number increases by the same amount.
    //
    // Anim 1: (z = 0)
    // Track 1 (y = 0): T|R|T|R|T|R|T|R|T|R|T|R|T|R|T|R
    // Track 2 (y = 1): T|R|T|R|T|R|T|R|T|R|T|R|T|R|T|R
    //
    // Anim 2: (z = 1)
    // Track 1 (y = 0): T|R|T|R|T|R|T|R|T|R|T|R|T|R|T|R
    // Track 2 (y = 1): T|R|T|R|T|R|T|R|T|R|T|R|T|R|T|R
    //
    m_maxFrames = 0;
    m_maxTracks = 0;
    m_maxAnims = 0;

    // Determine the maximum track length (X) and maximum number of tracks (Y).
    for(unsigned i = 0; i < m_animations.size(); i++)
    {
        Animation *anim = m_animations[i];
        if(!anim)
        {
            continue;
        }
        const BoneTrackSet &tracks = anim->tracks();
        unsigned numTracks = (uint32_t)tracks.size();
        for(unsigned j = 0; j < numTracks; j++)
        {
            const BoneTrack &track = tracks[j];
            m_maxFrames = qMax(track.frameCount, m_maxFrames);
        }
        m_maxTracks = qMax(numTracks, m_maxTracks);
    }
    m_maxAnims = m_animations.size();

    // Round up the texture width to help alignment.
    m_maxFrames = roundUp(m_maxFrames, 4);
}

bool AnimationArray::load(Animation **animations, size_t count)
{
    if(m_data)
    {
        clear();
    }
    m_animations.insert(m_animations.begin(), animations, animations + count);
    updateTextureDimensions();

    uint32_t animDataSize = m_maxFrames * m_maxTracks;
    m_data = new BoneTransform[animDataSize * m_maxAnims];
    for(unsigned z = 0; z < m_maxAnims; z++)
    {
        Animation *anim = m_animations[z];
        BoneTransform *animData = m_data + (animDataSize * z);
        if(anim)
        {
            anim->transformAll(animData, m_maxFrames);
        }
        else if(z > 0)
        {
            // Fill any missing animation data with the pose animation (z=0).
            memcpy(animData, m_data, animDataSize * sizeof(BoneTransform));
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void BoneTransform::clear()
{
    *this = identity();
}

vec3 BoneTransform::map(const vec3 &v) const
{
    return rotation.rotatedVec(v) + location;
}

vec4 BoneTransform::map(const vec4 &v) const
{
    vec3 v2 = map(v.asVec3());
    return vec4(v2.x, v2.y, v2.z, 1.0);
}

BoneTransform BoneTransform::map(const BoneTransform &t) const
{
    BoneTransform newT;
    newT.location = map(t.location);
    newT.rotation = vec4::multiply(rotation, t.rotation);
    return newT;
}

BoneTransform BoneTransform::identity()
{
    BoneTransform t;
    t.location = vec3(0.0f, 0.0f, 0.0f);
    t.rotation = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    t.padding = 0.0f;
    return t;
}

BoneTransform BoneTransform::interpolate(const BoneTransform &a,
                                         const BoneTransform &b,
                                         float f)
{
    BoneTransform c;
    c.rotation = vec4::slerp(a.rotation, b.rotation, f);
    c.location = (a.location * (1.0f - f)) + (b.location * f);
    return c;
}

void BoneTransform::toDualQuaternion(vec4 &d0, vec4 &d1) const
{
    const vec3 &tran(location);
    d0 = rotation;
    d1.x = 0.5f * (tran.x * d0.w + tran.y * d0.z - tran.z * d0.y);
    d1.y = 0.5f * (-tran.x * d0.z + tran.y * d0.w + tran.z * d0.x);
    d1.z = 0.5f * (tran.x * d0.y - tran.y * d0.x + tran.z * d0.w);
    d1.w = -0.5f * (tran.x * d0.x + tran.y * d0.y + tran.z * d0.z);
}

////////////////////////////////////////////////////////////////////////////////

BoneTransform BoneTrack::interpolate(double f) const
{
    int i = qRound(floor(f));
    BoneTransform result = BoneTransform::identity();
    if(frameCount > 0)
    {
        uint32_t index1 = (i % frameCount);
        uint32_t index2 = ((i + 1) % frameCount);
        const BoneTransform &trans1 = frames[index1];
        const BoneTransform &trans2 = frames[index2];
        result = BoneTransform::interpolate(trans1, trans2, (float)(f - i));
    }
    return result;
}
