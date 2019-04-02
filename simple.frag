
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    uv = vec2(fract(uv.x*10.0), 0.0);

    fragColor = vec4(uv, 0.0, 1.0);
}
