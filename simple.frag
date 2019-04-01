
float foo()
{
    return 0.5;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    if (!(uv.x > 0)) {
        uv.x = -uv.x;
    }

    fragColor = vec4(uv, 0.0, 1.0);
}
