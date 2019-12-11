// The following is a preamble needed on ShaderToy shaders to be
// compiled into the Vulkan environment of SPIR-V.

#version 450

precision highp float;

layout (binding = 0) uniform params {
	vec3 iResolution;            // viewport resolution in pixels
	float iTime;                 // Current time
	ivec4 iMouse;                // Mouse coords and click coords
        vec3 iChannelResolution[4];  // channel resolution (in pixels)
        int iFrame;                  // index of playback frame
}; // Params;

// layout (location = 0) in vec4 gl_FragCoord; // Builtin to Vulkan SPIR-V environment

layout (location = 0) out vec4 color;

#define distance distance_alice

float distance_alice(vec4 v1, vec4 v2)
{
    return sqrt(
        (v2.x - v1.x) * (v2.x - v1.x) + 
        (v2.y - v1.y) * (v2.y - v1.y) + 
        (v2.z - v1.z) * (v2.z - v1.z) + 
        (v2.w - v1.w) * (v2.w - v1.w));
}

float distance_alice(vec3 v1, vec3 v2)
{
    return sqrt(
        (v2.x - v1.x) * (v2.x - v1.x) + 
        (v2.y - v1.y) * (v2.y - v1.y) + 
        (v2.z - v1.z) * (v2.z - v1.z));
}

float distance_alice(vec2 v1, vec2 v2)
{
    return sqrt(
        (v2.x - v1.x) * (v2.x - v1.x) + 
        (v2.y - v1.y) * (v2.y - v1.y));
}

float distance_alice(float v1, float v2)
{
    return abs(v2 - v1);
}

#define length length_alice

float length_alice(vec4 v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

float length_alice(vec3 v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float length_alice(vec2 v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

float length_alice(float v)
{
    return abs(v);
}

#define normalize normalize_alice

vec4 normalize_alice(vec4 v)
{
    float d = sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    return v / d;
}

vec3 normalize_alice(vec3 v)
{
    float d = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return v / d;
}

vec2 normalize_alice(vec2 v)
{
    float d = sqrt(v.x * v.x + v.y * v.y);
    return v / d;
}

float normalize_alice(float v)
{
    return v;
}

#define cross cross_alice

vec3 cross(vec3 v1, vec3 v2)
{
    float x = v1.y*v2.z - v2.y*v1.z;
    float y = v1.z*v2.x - v2.z*v1.x;
    float z = v1.x*v2.y - v2.x*v1.y;
    return vec3(x, y, z);
}
