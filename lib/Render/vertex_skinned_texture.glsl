#extension GL_EXT_gpu_shader4 : require
attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec3 a_texCoords;
attribute vec4 a_color;
attribute float a_boneIndex; // to be compatible with OpenGL < 3.0

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

uniform int u_mapMaterialSlots;
uniform vec3 u_materialSlotMap[256];

uniform float u_fogStart;
uniform float u_fogEnd;
uniform float u_fogDensity;

uniform samplerBuffer u_animTex;
uniform vec3 u_animTexSize; // vec3(maxFrames*2, maxTracks, maxAnims)
uniform float u_animID;
uniform float u_animFrame;

varying vec3 v_color;
varying float v_texFactor;
varying vec3 v_texCoords;
varying float v_fogFactor;

// http://qt.gitorious.org/qt/qt/blobs/raw/4.7/src/gui/math3d/qquaternion.h
vec4 mult_quat(vec4 q1, vec4 q2)
{
    float ww = (q1.z + q1.x) * (q2.x + q2.y);
    float yy = (q1.w - q1.y) * (q2.w + q2.z);
    float zz = (q1.w + q1.y) * (q2.w - q2.z);
    float xx = ww + yy + zz;
    float qq = 0.5 * (xx + (q1.z - q1.x) * (q2.x - q2.y));

    float w = qq - ww + (q1.z - q1.y) * (q2.y - q2.z);
    float x = qq - xx + (q1.x + q1.w) * (q2.x + q2.w);
    float y = qq - yy + (q1.w - q1.x) * (q2.y + q2.z);
    float z = qq - zz + (q1.z + q1.y) * (q2.w - q2.x);
    return vec4(x, y, z, w);
}

// rotate the point v by the quaternion q
vec3 rotate_by_quat(vec3 v, vec4 q)
{
    return vec3(mult_quat(mult_quat(q, vec4(v, 0.0)), vec4(-q.x, -q.y, -q.z, q.w)));
}

vec4 nlerp(vec4 q1, vec4 q2, float a)
{
    vec4 q2abs = (dot(q1, q2) < 0.0) ? -q2 : q2;
    return normalize(mix(q1, q2abs, a));
}

vec4 skin(vec3 pos)
{
    // Determine the indices of the previous and next frames.
    float maxFrames = u_animTexSize.x * 0.5;
    float fi = floor(u_animFrame);
    float f1 = clamp(fi, 0.0, maxFrames - 1.0);
    float f2 = clamp(f1 + 1.0, 0.0, maxFrames - 1.0);
    
    // Look up the bone transformation for the previous and next frames.
    float animOffset = u_animID * u_animTexSize.x * u_animTexSize.y;
    float trackOffset = a_boneIndex * u_animTexSize.x;
    float transIndex1 = (f1 * 2.0) + trackOffset + animOffset;
    float rotIndex1 = ((f1 * 2.0) + 1.0) + trackOffset + animOffset;
    float transIndex2 = (f2 * 2.0) + trackOffset + animOffset;
    float rotIndex2 = ((f2 * 2.0) + 1.0) + trackOffset + animOffset;
    vec4 trans1 = texelFetch(u_animTex, int(transIndex1));
    vec4 rot1 = texelFetch(u_animTex, int(rotIndex1));
    vec4 trans2 = texelFetch(u_animTex, int(transIndex2));
    vec4 rot2 = texelFetch(u_animTex, int(rotIndex2));
  
    // Interpolate the translation and rotation parts of the bone transform.
    float alpha = (u_animFrame - fi);
    vec4 trans = mix(trans1, trans2, alpha);
    vec4 rot = nlerp(rot1, rot2, alpha);
    return vec4(rotate_by_quat(pos, rot) + trans.xyz, 1.0);
}

void main()
{
    vec4 viewPos = u_modelViewMatrix * skin(a_position);
    gl_Position = u_projectionMatrix * viewPos;
    
    // Transform texture coordinates if using the material map.
    float baseTex = a_texCoords.z - 1.0;
    vec2 baseTexCoords = a_texCoords.xy;
    vec3 matInfo = u_materialSlotMap[int(baseTex)];
    vec2 mappedTexCoords = baseTexCoords * matInfo.xy;
    float mappedTex = matInfo.z - 1.0;
    float finalTex = (u_mapMaterialSlots > 0) ? mappedTex : baseTex;
    vec2 finalTexCoords = (u_mapMaterialSlots > 0) ? mappedTexCoords : baseTexCoords;
    v_texCoords = vec3(finalTexCoords, finalTex);

    v_color = vec3(0.0, 0.0, 0.0);
    v_texFactor = 1.0;
    
    // Compute the fog factor.
    float fogFactor;
    float fogCoord = abs(viewPos.z / viewPos.w);
#ifndef EXP2_FOG
    fogFactor = (u_fogEnd - fogCoord) / (u_fogEnd - u_fogStart);
#else
    const float LOG2 = 1.442695;
    fogFactor = exp2(-pow(u_fogDensity * fogCoord, 2.0) * LOG2);
#endif
    v_fogFactor = (u_fogDensity > 0.0) ? 1.0 - clamp(fogFactor, 0.0, 1.0) : 0.0;
}
