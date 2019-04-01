
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    uv = floor(uv*10.0)/10.0;

    fragColor = vec4(uv, 0.0, 1.0);
}
