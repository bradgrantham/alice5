// https://www.shadertoy.com/view/3ttGRs
//

// A gnarly apollian tree
// Based upon: https://www.shadertoy.com/view/4ds3zn
#define PI  3.141592654

const int   max_iter      = 130;
const vec3  bone          = vec3(0.89, 0.855, 0.788);

void rot(inout vec2 p, float a) {
  float c = cos(a);
  float s = sin(a);
  p = vec2(c*p.x + s*p.y, -s*p.x + c*p.y);
}

float box(vec3 p, vec3 b) {
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float mod1(inout float p, float size) {
    float halfsize = size*0.5;
    float c = floor((p + halfsize)/size);
    p = mod(p + halfsize, size) - halfsize;
    return c;
}

vec2 modMirror2(inout vec2 p, vec2 size) {
    vec2 halfsize = size*0.5;
    vec2 c = floor((p + halfsize)/size);
    p = mod(p + halfsize, size) - halfsize;
    p *= mod(c,vec2(2))*2.0 - vec2(1.0);
    return c;
}

float apollian(vec3 p) {
  vec3 op = p;
  float s = 1.3 + smoothstep(0.15, 1.5, p.y)*0.95;
//  float s = 1.3 + min(pow(max(p.y - 0.25, 0.0), 1.0)*0.75, 1.5);
  float scale = 1.0;

  float r = 0.2;
  vec3 o = vec3(0.22, 0.0, 0.0);

  float d = 10000.0;

  const int rep = 7;

  for( int i=0; i<rep ;i++ ) {
    mod1(p.y, 2.0);
    modMirror2(p.xz, vec2(2.0));
    rot(p.xz, PI/5.5);

    float r2 = dot(p,p) + 0.0;
    float k = s/r2;
    float r = 0.5;
    p *= k;
    scale *= k;
  }

  d = box(p - 0.1, 1.0*vec3(1.0, 2.0, 1.0)) - 0.5;
  d = abs(d) - 0.01;
  return 0.25*d/scale;
}

float df(vec3 p) {
  float d1 = apollian(p);
  float db = box(p - vec3(0.0, 0.5, 0.0), vec3(0.75,1.0, 0.75)) - 0.5;
  float dp = p.y;
  return min(dp, max(d1, db));
}


float intersect(vec3 ro, vec3 rd, out int iter) {
  float res;
  float t = 0.2;
  iter = max_iter;

  for(int i = 0; i < max_iter; ++i) {
    vec3 p = ro + rd * t;
    res = df(p);
    if(res < 0.0003 * t || res > 20.) {
      iter = i;
      break;
    }
    t += res;
  }

  if(res > 20.) t = -1.;
  return t;
}

float ambientOcclusion(vec3 p, vec3 n) {
  float stepSize = 0.012;
  float t = stepSize;

  float oc = 0.0;

  for(int i = 0; i < 12; i++) {
    float d = df(p + n * t);
    oc += t - d;
    t += stepSize;
  }

  return clamp(oc, 0.0, 1.0);
}

vec3 normal(in vec3 pos) {
  vec3  eps = vec3(.001,0.0,0.0);
  vec3 nor;
  nor.x = df(pos+eps.xyy) - df(pos-eps.xyy);
  nor.y = df(pos+eps.yxy) - df(pos-eps.yxy);
  nor.z = df(pos+eps.yyx) - df(pos-eps.yyx);
  return normalize(nor);
}

vec3 lighting(vec3 p, vec3 rd, int iter) {
  vec3 n = normal(p);
  float fake = float(iter)/float(max_iter);
  float fakeAmb = exp(-fake*fake*9.0);
  float amb = ambientOcclusion(p, n);

  vec3 col = vec3(mix(1.0, 0.125, pow(amb, 3.0)))*vec3(fakeAmb)*bone;
  return col;
}

vec3 post(vec3 col, vec2 q) {
  col=pow(clamp(col,0.0,1.0),vec3(0.65));
  col=col*0.6+0.4*col*col*(3.0-2.0*col);  // contrast
  col=mix(col, vec3(dot(col, vec3(0.33))), -0.5);  // satuation
  col*=0.5+0.5*pow(19.0*q.x*q.y*(1.0-q.x)*(1.0-q.y),0.7);  // vigneting
  return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )  {
  vec2 q=fragCoord.xy/iResolution.xy;
  vec2 uv = -1.0 + 2.0*q;
  uv.y += 0.225;
  uv.x*=iResolution.x/iResolution.y;

  vec3 la = vec3(0.0, 0.5, 0.0);
  vec3 ro = vec3(-4.0, 1., -0.0);
  rot(ro.xz, 2.0*PI*iTime/120.0);
  vec3 cf = normalize(la-ro);
  vec3 cs = normalize(cross(cf,vec3(0.0,1.0,0.0)));
  vec3 cu = normalize(cross(cs,cf));
  vec3 rd = normalize(uv.x*cs + uv.y*cu + 3.0*cf);  // transform from view to world

  vec3 bg = mix(bone*0.5, bone, smoothstep(-1.0, 1.0, uv.y));
  vec3 col = bg;

  vec3 p=ro;

  int iter = 0;

  float t = intersect(ro, rd, iter);

  if(t > -0.5) {
    p = ro + t * rd;
    col = lighting(p, rd, iter);
    col = mix(col, bg, 1.0-exp(-0.001*t*t));
  }


  col=post(col, q);
  fragColor=vec4(col.x,col.y,col.z,1.0);
}
