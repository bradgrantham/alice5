
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    // -1 to 1.
    float x = uv.x*2.0;

    // -1 to 1.
    float y = cos(uv.x*10.0);

    uv = vec2(1.0, 1.0)*(uv.y < y/2.0 ? 1.0 : 0.0);

    fragColor = vec4(uv, 0.0, 1.0);
}
