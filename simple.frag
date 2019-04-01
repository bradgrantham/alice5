
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    uv = cross(normalize(vec3(uv, 0.0)), vec3(0.0, 0.0, -1.0)).xy;

    fragColor = vec4(uv, 0.0, 1.0);
}
