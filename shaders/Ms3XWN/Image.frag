// Created by inigo quilez - iq/2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0


// postprocess (thanks to Timothy Lottes for the CRT filter - https://www.shadertoy.com/view/XsjSzR)



float sdCircle( in vec2 p, in float r )
{
    return length( p ) - r;
}

vec4 loadValue( in ivec2 re )
{
    return texelFetch( iChannel0, re, 0 );
}

vec2 dir2dis( float dir )
{
    vec2 off = vec2(0.0);
         if( dir<0.5 ) { off = vec2( 0.0, 0.0); }
    else if( dir<1.5 ) { off = vec2( 1.0, 0.0); }
    else if( dir<2.5 ) { off = vec2(-1.0, 0.0); }
    else if( dir<3.5 ) { off = vec2( 0.0, 1.0); }
    else               { off = vec2( 0.0,-1.0); }
    return off;
}

vec2 cell2ndc( vec2 c )
{
	c = (c+0.5) / 31.0;
    c.x -= 0.5*(1.0-iResolution.x/iResolution.y); // center
    return c;
}


vec3 drawMap( vec3 col, in vec2 fragCoord )
{
    vec2 p = fragCoord/iResolution.y;
    p.x += 0.5*(1.0-iResolution.x/iResolution.y); // center
    float wp = 1.0/iResolution.y;

    p *= 31.0;
    vec2 q = floor(p);
    vec2 r = fract(p);
    float wr = 31.0*wp;

    if( q.x>=0.0 && q.x<=27.0 )
    {
        float c = texture( iChannel0, (q+0.5)/iResolution.xy, -100.0 ).x;

        // points
        if( abs(c-2.0)<0.5 )
        {
            float d = sdCircle(r-0.5, 0.15);
            col += 0.3*vec3(1.0,0.7,0.4)*exp(-22.0*d*d); // glow
        }
    }
    
	// balls
    
    vec2 bp[4];
    
    bp[0] = vec2( 1.0, 7.0) + 0.5;
    bp[1] = vec2(25.0, 7.0) + 0.5;
    bp[2] = vec2( 1.0,27.0) + 0.5;
    bp[3] = vec2(25.0,27.0) + 0.5;
    
    for( int i=0; i<4; i++ )
    {
        float c = texture( iChannel0, (bp[i]+0.5)/iResolution.xy, -100.0 ).x;
        if( abs(c-3.0)<0.5 )
        {
        float d = length(p - bp[i]);
        col += 0.35*vec3(1.0,0.7,0.4)*exp(-1.0*d*d)*smoothstep( -1.0, -0.5, sin(2.0*6.2831*iTime) );
        }
    }
    
    return col;
}


vec3 drawPacman( vec3 col, in vec2 fragCoord, in vec4 pacmanPos, in vec3 pacmanMovDirNex )
{
    vec2 off = dir2dis(pacmanMovDirNex.x);
    
    vec2 mPacmanPos = pacmanPos.xy;
    //vec2 mPacmanPos = pacmanPos.xy + off*pacmanPos.z*pacmanPos.w;

    vec2 p = fragCoord/iResolution.y;
    float eps = 1.0 / iResolution.y;

    vec2 q = p - cell2ndc( mPacmanPos );

    float c = max(0.0,sdCircle(q, 0.023));

    // glow
    col += 0.25*vec3(1.0,0.8,0.0)*exp(-400.0*c*c);

    return col;
}

vec3 drawGhost( vec3 col, in vec2 fragCoord, in vec3 pos, in float dir, in float id, in vec3 mode )
{
    vec2 off = dir2dis(dir);

    vec2 gpos = pos.xy;

    vec2 p = fragCoord/iResolution.y;
    float eps = 1.0 / iResolution.y;

    vec2 q = p - cell2ndc( gpos );

    float c = max(0.0,sdCircle(q, 0.023));
   
    vec3 gco = 0.5 + 0.5*cos( 5.0 + 0.7*id + vec3(0.0,2.0,4.0) );
    float g = mode.x;
    if( mode.z>0.75 )
    {
        g *= smoothstep(-0.2,0.0,sin(3.0*6.28318*(iTime-mode.y)));
    }
    gco = mix( gco, vec3(0.1,0.5,1.0), g );

    // glow
    col += 0.2*gco*exp(-300.0*c*c);

    return col;
}

vec3 drawScore( in vec3 col, in vec2 fragCoord, vec2 score, float lives )
{
    // score
    vec2 p = fragCoord/iResolution.y;
    // lives
    float eps = 1.0 / iResolution.y;
    for( int i=0; i<3; i++ )
    {
        float h = float(i);
        vec2 q = p - vec2(0.1 + 0.075*h, 0.7 );
        if( h + 0.5 < lives )
        {
            float c = max(0.0,sdCircle(q, 0.023));

            col += 0.17*vec3(1.0,0.8,0.0)*exp(-1500.0*c*c);
        }
    }

    return col;
}

//============================================================

//
// PUBLIC DOMAIN CRT STYLED SCAN-LINE SHADER
//
//   by Timothy Lottes
//
// This is more along the style of a really good CGA arcade monitor.
// With RGB inputs instead of NTSC.
// The shadow mask example has the mask rotated 90 degrees for less chromatic aberration.
//
// Left it unoptimized to show the theory behind the algorithm.
//
// It is an example what I personally would want as a display option for pixel art games.
// Please take and use, change, or whatever.
//

// Emulated input resolution.

//vec2 res = 640.0*vec2(1.0,iResolution.y/iResolution.x);
#define res (iResolution.xy/floor(1.0+iResolution.xy/512.0))

// Hardness of scanline.
//  -8.0 = soft
// -16.0 = medium
const float hardScan=-8.0;

// Hardness of pixels in scanline.
// -2.0 = soft
// -4.0 = hard
const float hardPix=-3.0;

// Display warp.
// 0.0 = none
// 1.0/8.0 = extreme
const vec2 warp=vec2(1.0/32.0,1.0/24.0); 

// Amount of shadow mask.
const float maskDark=0.6;
const float maskLight=2.0;

//------------------------------------------------------------------------

// sRGB to Linear.
// Assuing using sRGB typed textures this should not be needed.
float ToLinear1(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}
vec3 ToLinear(vec3 c){return vec3(ToLinear1(c.r),ToLinear1(c.g),ToLinear1(c.b));}

// Linear to sRGB.
// Assuing using sRGB typed textures this should not be needed.
float ToSrgb1(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}
vec3 ToSrgb(vec3 c){return vec3(ToSrgb1(c.r),ToSrgb1(c.g),ToSrgb1(c.b));}

// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
vec3 Fetch(vec2 pos,vec2 off){
  pos=floor(pos*res+off)/res;
  if(max(abs(pos.x-0.5),abs(pos.y-0.5))>0.5)return vec3(0.0,0.0,0.0);
  return ToLinear(texture(iChannel1,pos.xy,-16.0).rgb);}

// Distance in emulated pixels to nearest texel.
vec2 Dist(vec2 pos){pos=pos*res;return -((pos-floor(pos))-vec2(0.5));}
    
// 1D Gaussian.
float Gaus(float pos,float scale){return exp2(scale*pos*pos);}

// 3-tap Gaussian filter along horz line.
vec3 Horz3(vec2 pos,float off){
  vec3 b=Fetch(pos,vec2(-1.0,off));
  vec3 c=Fetch(pos,vec2( 0.0,off));
  vec3 d=Fetch(pos,vec2( 1.0,off));
  float dst=Dist(pos).x;
  // Convert distance to weight.
  float scale=hardPix;
  float wb=Gaus(dst-1.0,scale);
  float wc=Gaus(dst+0.0,scale);
  float wd=Gaus(dst+1.0,scale);
  // Return filtered sample.
  return (b*wb+c*wc+d*wd)/(wb+wc+wd);}

// 5-tap Gaussian filter along horz line.
vec3 Horz5(vec2 pos,float off){
  vec3 a=Fetch(pos,vec2(-2.0,off));
  vec3 b=Fetch(pos,vec2(-1.0,off));
  vec3 c=Fetch(pos,vec2( 0.0,off));
  vec3 d=Fetch(pos,vec2( 1.0,off));
  vec3 e=Fetch(pos,vec2( 2.0,off));
  float dst=Dist(pos).x;
  // Convert distance to weight.
  float scale=hardPix;
  float wa=Gaus(dst-2.0,scale);
  float wb=Gaus(dst-1.0,scale);
  float wc=Gaus(dst+0.0,scale);
  float wd=Gaus(dst+1.0,scale);
  float we=Gaus(dst+2.0,scale);
  // Return filtered sample.
  return (a*wa+b*wb+c*wc+d*wd+e*we)/(wa+wb+wc+wd+we);}

// Return scanline weight.
float Scan(vec2 pos,float off){
  float dst=Dist(pos).y;
  return Gaus(dst+off,hardScan);}

// Allow nearest three lines to effect pixel.
vec3 Tri(vec2 pos){
  vec3 a=Horz3(pos,-1.0);
  vec3 b=Horz5(pos, 0.0);
  vec3 c=Horz3(pos, 1.0);
  float wa=Scan(pos,-1.0);
  float wb=Scan(pos, 0.0);
  float wc=Scan(pos, 1.0);
  return a*wa+b*wb+c*wc;}


// Shadow mask.
vec3 Mask(vec2 pos)
{
  pos.x+=pos.y*3.0;
  vec3 mask=vec3(maskDark,maskDark,maskDark);
  pos.x=fract(pos.x/6.0);
  if(pos.x<0.333)mask.r=maskLight;
  else if(pos.x<0.666)mask.g=maskLight;
  else mask.b=maskLight;
  return mask;}    


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    //------------------------
    // CRT
    //------------------------
    vec3 col = ToSrgb( Tri(fragCoord.xy/iResolution.xy)*Mask(fragCoord.xy) );
    //col = texture( iChannel1, fragCoord.xy/iResolution.xy ).xyz;

    //------------------------
    // glow
    //------------------------
       
    vec4  pacmanPos = loadValue( txPacmanPos );
    vec3  pacmanDir = loadValue( txPacmanMovDirNex ).xyz;
    vec4  ghostPos[4];
    ghostPos[0]     = loadValue( txGhost0PosDir );
    ghostPos[1]     = loadValue( txGhost1PosDir );
    ghostPos[2]     = loadValue( txGhost2PosDir );
    ghostPos[3]     = loadValue( txGhost3PosDir );
    vec2  points    = loadValue( txPoints ).xy;
    float state     = loadValue( txState ).x;
    float lives     = loadValue( txLives ).x;
    vec3 mode       = loadValue( txMode ) .xyz;

    // map
    col = drawMap( col, fragCoord );

    // pacman
    col = drawPacman( col, fragCoord, pacmanPos, pacmanDir );

    // ghosts
    for( int i=0; i<4; i++ )
        col = drawGhost( col, fragCoord, ghostPos[i].xyz, ghostPos[i].w, float(i), mode );
    
    // score
    col = drawScore( col, fragCoord, points, lives );
    
	fragColor = vec4( col, 1.0 );
}