void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 uv = gl_FragCoord / vec4(320, 180, 1, 1);
    float a = atan(-.5 + uv.x, -.5 + uv.y);
    float q = a * 10;
    float colorScale = 2;
    float colorOffset = (colorScale - 1) / colorScale;
    float cosine = cos(q.x) / colorScale + colorOffset;
    fragColor = vec4(cosine, cosine, 0, 0);
}
