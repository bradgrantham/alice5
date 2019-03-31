// https://www.shadertoy.com/view/tsjXDh

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

	//uv.x *= iResolution.x / iResolution.y;

    uv = uv*2. - 1.;

    float da = iTime/2.;

    vec3 col = vec3(1.);
    float d = length(abs(uv)-atan(uv)*5.);
    float a = atan(uv.x,uv.y);
    d = sin(a * 10.) * cos(a * 10.);
    d -= length(abs(uv)- sin(da)*5.);

    col *= smoothstep(0.4, 0.41, length(abs(uv)));
    col = vec3(fract(d*dot(uv.x,uv.y)));

    col += vec3(.3, 0., 1.0);
    // Output to screen
    fragColor = vec4(col,1.0);
}
