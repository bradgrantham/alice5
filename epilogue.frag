// The following is an epilogue needed on ShaderToy shaders to be
// compiled into the Vulkan environment of SPIR-V.

void main()
{
    mainImage(color, gl_FragCoord.xy);
}

