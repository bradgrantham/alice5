
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    // -0.5 to 0.5
    float t = min(uv.x, -0.2);

    uv = vec2(1.0, 1.0)*(uv.y < t ? 1.0 : 0.0);

    fragColor = vec4(uv, 0.0, 1.0);
}
