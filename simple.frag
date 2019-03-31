void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    vec2 inside = vec2(0.2, 0.4);
    vec2 outside = vec2(0.6, 0.8);

    float d = uv.x*uv.x + uv.y*uv.y;
    uv = vec2(0.0);
    for (int i = 0; i < 10; i++) {
        if (d <= 0.5*0.5) {
            uv += inside*0.1;
        } else {
            uv += outside*0.1;
        }
    }

    fragColor = vec4(uv, 0.0, 1.0);
}
