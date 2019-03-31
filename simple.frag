void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    uv.x = uv.x + 0.5;

    // Output to screen
    fragColor = vec4(uv, 1.0, 1.0);
}
