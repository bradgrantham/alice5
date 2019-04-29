// https://www.shadertoy.com/view/MtBBRW

#define M_PI 3.14159266

#define ROTATIONS_PER_SECOND (1.0 / 37.345)

#define ZOOMS_PER_SECOND (1.0 / 23.384)
#define MAX_ZOOM 5.0
#define MIN_ZOOM 0.8

#define DX_PER_SECOND (1.0 / 25.894)
#define DY_PER_SECOND (1.0 / 17.127)
#define MAX_DX 0.3
#define MAX_DY 0.3

float swirl(in vec2 p, in int m, in float s)
{
    float d;
    
    if (p == vec2(0.0,0.0)) {
        return 0.0;
    }
    
    d = length(p);

    return cos(float(m)*(atan(p.y, p.x) + s*d))*0.5 + 0.5;
}

float sink(in vec2 p, in float r)
{
    float d = length(p);
    
    d = d/r - 1.0;
    d = max(d, 0.0);

    return 1.0 - 1.0/exp(d);
}

float ripple(in vec2 p, in float r)
{
    float d = length(p);

    return cos(d/r*M_PI)*0.5 + 0.5;
}

vec2 bulge(in vec2 p, in vec2 c, float r, float e)
{
    vec2 v;
    float d;
    
    v = p - c;
    d = length(v);
    if (d == 0.0) {
        return p;
    }
    
    v /= d;
    
    d = pow(d/r, e)*r;
    return c + v * d;
}

float fn(in vec2 p)
{
    float f;
    float a;
    vec2 d;
    
    p.y = 1.0 - p.y;
    
    d = p;
    
    p = bulge(p, vec2(0.5, 0.7), 0.6, 3.0);
    
    a = 0.1;
    f = ripple(d - vec2(0.4, 0.4), a)
        + ripple(d - vec2(0.8, 0.1), a)
        + ripple(d - vec2(0.9, 0.3), a)
        + ripple(d - vec2(0.3, 0.8), a)
        + ripple(d - vec2(0.7, 0.1), a)
        + ripple(d - vec2(0.4, 0.5), a);
    f /= 6.0;
    
    a *= sink(d - vec2(0.5, 0.7), 0.4);
    if (a > 0.0) {
        p.x += cos(f*2.0*M_PI*2.0)*a;
        p.y += sin(f*2.0*M_PI*2.0)*a;
    }
    
    a = 0.01;
    f = ripple(p - vec2(0.2, 0.4), a)
        + ripple(p - vec2(0.4, 0.2), a)
        + ripple(p - vec2(0.6, 0.8), a)
        + ripple(p - vec2(0.1, 0.2), a)
        + ripple(p - vec2(0.9, 0.2), a)
        + ripple(p - vec2(0.2, 0.3), a);
    f /= 6.0;
    
    p.x += cos(f*2.0*M_PI*2.0)*a;
    p.y += sin(f*2.0*M_PI*2.0)*a;
    
    f = pow(swirl(p - vec2(0.5, 0.7), 3, 12.0), 0.7);
    f *= sink(p - vec2(0.5, 0.7), 0.005);

    return f;
}

vec2 rotate(in vec2 p, in vec2 origin, in float rad)
{
    float s = sin(rad);
    float c = cos(rad);
    
    // We don't support matrices in the compiler.
    /*
    mat2 m = mat2(c, -s, s, c);

    return m * (p - origin) + origin;
    */

    vec2 v1 = vec2(c, s);
    vec2 v2 = vec2(-s, c);
    vec2 v = p - origin;
    vec2 r = vec2(dot(v1, v), dot(v2, v));
    return r + origin;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.y;
    uv.x -= iResolution.x / (4.0 * iResolution.y);
    
    // Rotate around the center of interest at the desired rate.
    vec2 rotateAround = vec2(0.5, 0.5);
    uv = rotate(uv, rotateAround, iTime * 2.0 * M_PI * ROTATIONS_PER_SECOND);
    
    // Get a pair of sine waves with the desired periods and ranges for translation.
    vec2 d;
    d.x = sin(iTime * 2.0 * M_PI * DX_PER_SECOND) * MAX_DX;
    d.y = sin(iTime * 2.0 * M_PI * DY_PER_SECOND) * MAX_DY;
    uv += d;
    
    // Get a sine wave with the desired period on the range [0,1], starting zoomed out.
    float zoom = (-cos(iTime * 2.0 * M_PI * ZOOMS_PER_SECOND) + 1.0) / 2.0;
    uv /= mix(MIN_ZOOM, MAX_ZOOM, zoom);
    
    float f = fn(uv);
    
    fragColor = vec4(f, f, f, 1.0);
}
