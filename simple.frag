
float foo()
{
    return 0.5;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;
    mat2 m = mat2(1.0, 0.0, 0.0, 1.0);
    m = -m;


    fragColor = vec4(uv, 0.0, 1.0);
}
