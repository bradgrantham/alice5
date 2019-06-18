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

