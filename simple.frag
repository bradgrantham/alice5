
layout (location = 1) in vec3 inv1;
layout (location = 2) in vec3 inv2;
layout (location = 3) out vec3 outv;
layout (location = 4) out float outf;
layout (location = 5) in float inf;

/*
float foo(float n) {
    float i = 1;
    float j = 0;

    while (i <= n) {
        j += i;
        i += 1;
    }

    return j;
}
*/

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    /*
    vec2 uv = (fragCoord - iResolution.xy/2.0)/iResolution.y;

    // Plot a function.
    if (false) {
        // -1 to 1.
        float x = uv.x*2.0;

        // -1 to 1.
        float y = mix(0.8, -0.8, (x + 1.0)/2.0);

        uv = vec2(1.0, 1.0)*(uv.y < y/2.0 ? 1.0 : 0.0);
    }

    // Vector test.
    if (false) {
        uv = normalize(uv);
        vec2 n = vec2(0.0, 1.0);
        uv = reflect(uv, n);
    }

    // Matrix test.
    if (false) {
        float a = 10.0*3.14159/180.0;
        float s = sin(a);
        float c = cos(a);
        mat2 m = mat2(c, s, -s, c);

        uv = uv*m;

        // Checkerboard.
        uv = floor(mod(uv*vec2(10.0), 2.0));
    }

    // fragColor = vec4(uv, 0.0, 1.0);
    */

    vec3 x;

    if (inf < 0) {
        x = inv1;
    } else {
        x = inv2;
    }

    outv = x;
}
