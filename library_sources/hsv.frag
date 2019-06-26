void colorHSVToRGB3f(in vec3 hsv, out vec3 rgb)
{
    if(hsv.y < .00001) {

        rgb = vec3(hsv.z, hsv.z, hsv.z);

    } else {

        float h = hsv.x;
        float s = hsv.y;
        float v = hsv.z;

#if 1
	h = mod(h, 3.14159265259 * 2.0);	/* wrap just in case */

        float where = floor(h / (3.14159265259 / 3.0));

	float f = mod(h, 3.14159265259 / 3.0);

	float p = v * (1.0 - s);
	float q = v * (1.0 - s * f);
	float t = v * (1.0 - s * (1.0 - f));

	if(where < 0.5) {
            rgb = vec3(v, t, p);
        } else if(where < 1.5) {
            rgb = vec3(q, v, p);
        } else if(where < 2.5) {
            rgb = vec3(p, v, t);
        } else if(where < 3.5) {
            rgb = vec3(p, q, v);
        } else if(where < 4.5) {
            rgb = vec3(t, p, v);
        } else {
            rgb = vec3(v, p, q);
	}
#else
        // DEBUG
        h = (h + 3.14159265259) / (3.14159265259 * 2.0);
        rgb = vec3(h,h,h);
#endif
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 uv = gl_FragCoord / vec4(320, 180, 1, 1);
    vec4 xy = uv - vec4(.5, .5, 0, 0);
    float a = xy.x * 3.1415926259 * 2;
    float x = cos(a);
    float y = sin(a);
    float a2 = atan(y, x);
    vec3 hsv = vec3(a2, 1, 1);
    vec3 rgb = vec3(.5, .5, .5);
    colorHSVToRGB3f(hsv, rgb);
    fragColor = vec4(rgb, 1);
}
