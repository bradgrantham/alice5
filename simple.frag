
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    float a = (smoothstep(0.25, -0.25, uv.x) - 0.5)*0.5;

    uv = vec2(1.0)*float(uv.y < a);

    fragColor = vec4(uv, 0.0, 1.0);
}
