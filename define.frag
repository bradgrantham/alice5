//////////////////////////////////////////////////////////////////////////////////
// 1 Tweet Spiral Machine - By Frank Force - Copyright 2019
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//////////////////////////////////////////////////////////////////////////////////

#define S(v)step(.5,fract(v*a*t))
#define mainImage(c,p)vec2 r=iResolution.xy,u=99.*(p.xy-r/2.)/r.x;float P=6.28,t=.01*iTime,A=atan(u.y,u.x),a=A+P*floor(length(u)-A/P+.5),s=S(.5),v=S(1.);c=vec4(v-s*v*.5)+vec4(s*v)*cos(P*(.002*a+t+vec4(1.,.6,.3,1.)));

/*
#define S(v) step(.5, fract(v*a*t))
#define mainImage(c,p)\t\t\t\t\t\t\t\t\\
    vec2 r = iResolution.xy,\t\t\t\t\t\t\\
        u = 99.*(p.xy - r / 2.)/ r.x;\t\t\t\t\\
    float P = 6.28,\t\t\t\t\t\t\t\t\t\\
          t = .01*iTime,\t\t\t\t\t\t\t\\
          A = atan(u.y, u.x),\t\t\t\t\t\t\\
          a = A + P*floor(length(u) - A/P + .5),\t\\
          s = S(.5),\t\t\t\t\t\t\t\t\\
          v = S(1.);\t\t\t\t\t\t\t\t\\
    c = vec4(v - s * v * .5) + vec4(s * v) * \t\t\\
      cos(P * (.002*a + t + vec4(1., .6, .3, 1.)));
*/
