// The following is a preamble needed on ShaderToy shaders to be
// compiled into the Vulkan environment of SPIR-V.

#version 450

precision highp float;

layout (binding = 0) uniform params {
	ivec2 iResolution;
} Params;

layout (location = 0) in vec2 fragCoord;

layout (location = 0) out vec4 color;

void mainImage( out vec4 fragColor, in vec2 fragCoord );

void main()
{
    mainImage(color, fragCoord);
}

// Thus endeth the preamble

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/Params.iResolution.xy; // XXX bradgrantham had to add "Params."

    // Output to screen
    fragColor = vec4(uv, 1.0, 1.0);
}
