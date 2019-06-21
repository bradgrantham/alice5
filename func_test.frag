
float domain_min = -3.14159 * 5;
float domain_max = 3.14159 * 5;
float domain_tick = 3.14159;

float range_min = -2;
float range_max = 2;
float range_tick = 1.0;

float f(float x)
{
    return sin(x);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 u0v0 = gl_FragCoord / vec4(320, 180, 1, 1);
    vec4 u1v0 = (gl_FragCoord + vec4(1, 0, 0, 0)) / vec4(320, 180, 1, 1);
    vec4 u0v1 = (gl_FragCoord + vec4(0, 1, 0, 0)) / vec4(320, 180, 1, 1);

    float x0 = domain_min + (domain_max - domain_min) * u0v0.x;
    float x1 = domain_min + (domain_max - domain_min) * u1v0.x;

    float y0 = range_min + (range_max - range_min) * u0v0.y;
    float y1 = range_min + (range_max - range_min) * u0v1.y;

    float f_x0 = f(x0);
    float f_x1 = f(x1);

    vec3 baseColor = vec3(0, 0, 0);

    if((x0 < 0) && (x1 >= 0)) {

        baseColor = vec3(1, 1, 1);

    } else if((y0 < 0) && (y1 >= 0)) {

        baseColor = vec3(1, 1, 1);

    } else if(mod(x0, domain_tick) > mod(x1, domain_tick)) {

        baseColor = vec3(.5, .5, .5);

    } else if(mod(y0, range_tick) > mod(y1, range_tick)) {

        baseColor = vec3(.5, .5, .5);
    }

    bvec4 signs = bvec4(y0 < f_x0, y1 < f_x0, y0 < f_x1, y1 < f_x1);
    vec3 plotColor;
    if(any(signs) && !all(signs))
        plotColor = vec3(1, .5, .5);
    else
        plotColor = vec3(0, 0, 0);

    fragColor = vec4(plotColor + baseColor, 1.0f);
}
