
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    uv = vec2(atan(uv.y/uv.x), atan(uv.y, uv.x))/3.1415;

    fragColor = vec4(uv, 0.0, 1.0);
}
