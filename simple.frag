
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    uv.x = exp2(uv.x*2.0)/2;
    uv.y = 0.0;

    fragColor = vec4(uv, 0.0, 1.0);
}
