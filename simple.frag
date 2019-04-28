
layout (location = 1) in vec3 intest;
layout (location = 2) out vec3 outtest;
layout (location = 3) out float outtestf;
layout (location = 4) in float intestf;

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

    //fragColor.x = foo(10.0);
    // fragColor.x = 0.4;
    // fragColor.y = 0.5;
    // fragColor.z = 0.6;
    // outtest = 0.0;
    // outtest = 1.0 + outtest;
    // fragColor = vec4(outtest, outtest, outtest, 1.0);
    // vec3 q = vec3(0.4, 0.5, 0.6);
    // fragColor = vec4(cross(q, q*2.0), 1.0);
    // fragColor = vec4(1.0, 2.0, 3.0, 4.0);
    // outtest = cross(intest, intest);

    // outtestf = 1.2;
    outtest = normalize(intest);

    // vec3 x = vec3(1.0, 2.0, 3.0);
    // vec3 y = vec3(4.0, 5.0, 6.0);
    // vec3 z = x + y;

    // swizzle test
    if(false) {
        vec4 p;
        p.xy = vec2(2.0, 3.0);
        p.zw = vec2(5.0, 7.0);
    }
}
