#define MAX_STEPS 100
#define MAX_DIST 100.0
#define SURF_DIST 0.01

mat4 rotateY(float theta) {
    float c = cos(theta);
    float s = sin(theta);

    return mat4(
        vec4(c, 0, s, 0),
        vec4(0, 1, 0, 0),
        vec4(-s, 0, c, 0),
        vec4(0, 0, 0, 1)
    );
}

// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm

float sphere_dist(vec3 p, vec4 s) {
    return distance(p, s.xyz) - s.w;
}

float box_dist(vec3 p, vec3 c, vec3 s, float r) {
    vec3 d = abs(p - c) - (s - r);
    return length(max(d, 0.0)) - r
        + min(max(d.x, max(d.y, d.z)), 0.0);
}

float union_d(float d1, float d2) {
    return min(d1, d2);
}

float intersection_d(float d1, float d2) {
    return max(d1, d2);
}

float difference_d(float d1, float d2) {
    return max(d1, -d2);
}

float pMod1(inout float p, float size) {
	float halfsize = size*0.5;
	float c = floor((p + halfsize)/size);
	p = mod(p + halfsize, size) - halfsize;
	return c;
}

// (distance,oid)
// oid -1.0 = miss
// oid 0.0 = floor
// oid 1.0 = main object
vec2 dist(vec3 p, bool includeFloor) {
    float sd = sphere_dist(p, vec4(0.0, 0.0, 0.0, 3.0));
    float bd = box_dist(p, vec3(0.0, 0.0, 0.0), vec3(2.5), 0.5);
    
    float od = difference_d(bd, sd);
    
    if (includeFloor) {
	    float fd = p.y - -3.0;
    	if (fd < od) {
        	return vec2(fd, 0.0);
    	}
    }

    return vec2(od, 1.0);
}

vec2 march(vec3 ro, vec3 rd) {
    float d = 0.0;
    
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + d*rd;
        vec2 f = dist(p, true);
        d += f.x;
        if (d > MAX_DIST || f.x < SURF_DIST) {
            return vec2(d, f.y);
        }
    }
    
    return vec2(0.0, -1.0);
}

vec3 normal(vec3 p) {
    vec2 e = vec2(0.01, 0);
    
    return normalize(
        dist(p, true).x - vec3(
            dist(p - e.xyy, true).x,
            dist(p - e.yxy, true).x,
            dist(p - e.yyx, true).x));
}

vec3 shade(vec3 rd, vec3 p, int oid) {
    vec3 l = vec3(0.0, 10.0, 0.0);
    l.xz += vec2(sin(iTime+3.0), cos(iTime+3.0))*10.0;
    
    vec3 to_l = normalize(l - p);
    vec3 n = normal(p);
    float diffuse = max(0.0, dot(to_l, n));
    vec3 col;
    if (oid == 0) {
        // Floor.
        float d = dist(p, false).x;
        d = mod(d, 1.0) > 0.9 ? 1.0 : 0.9;
        col = vec3(0.8, 0.8, 0.8)*diffuse*d;
    } else if (oid == 1) {
        // Object.
        vec3 r = 2.0*dot(to_l, n)*n - to_l;
        float specular = pow(max(dot(r, -rd), 0.0), 5.0);
        col = vec3(1.0, 0.0, 0.0)*diffuse + vec3(1.0, 1.0, 1.0)*specular;
    } else {
        col = vec3(1.0, 1.0, 0.0);
    }
    
    // Shadow.
    if (march(p + n*SURF_DIST*2.0, to_l).x < distance(l, p)) {
        col *= 0.5;
    }
    
    return col;
}

mat3 makeCamera(vec3 eye, vec3 lookat) {
    vec3 z = normalize(eye - lookat);
    vec3 x = normalize(cross(vec3(0.0, 1.0, 0.0), z));
    vec3 y = cross(z, x);
    
    return mat3(x, y, z);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord )
{
    vec2 xy = (fragCoord - iResolution.xy/2.0)/iResolution.y;
    
    vec3 eye = vec3(sin((iTime + 3.0)/10.0)*10.0, 7.0, cos((iTime + 3.0)/10.0)*10.0);
    vec3 lookat = vec3(0.0, 0.0, 0.0);
    mat3 cam = makeCamera(eye, lookat);
        
    vec3 rd = normalize(-cam[2]*1.5 + cam[0]*xy.x + cam[1]*xy.y);
    vec3 ro = eye;
        
    vec2 hit = march(ro, rd);
    vec3 col;
    if (hit.y == -1.0) {
        col = vec3(1.0, 0.0, 1.0);
    } else {
	    vec3 p = ro + rd*hit.x;
    	col = shade(rd, p, int(hit.y));
    }
    
    fragColor = vec4(col, 1.0);
}
