// Created by inigo quilez - iq/2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0


// rendering


float sdBox( vec2 p, vec2 b )
{
  vec2 d = abs(p) - b;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float sdBox( vec2 p, vec2 a, vec2 b )
{
  p -= (a+b)*0.5;
  vec2 d = abs(p) - 0.5*(b-a);
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float sdCircle( in vec2 p, in float r )
{
    return length( p ) - r;
}

//============================================================

// digit data by P_Malin (https://www.shadertoy.com/view/4sf3RN)
// converted to LUT and integer logic by iq
const int[] font = int[]( 
    7 + 5*16 + 5*256 + 5*4096 + 7*65536,
    2 + 2*16 + 2*256 + 2*4096 + 2*65536,
    7 + 1*16 + 7*256 + 4*4096 + 7*65536,
    7 + 4*16 + 7*256 + 4*4096 + 7*65536,
    4 + 7*16 + 5*256 + 1*4096 + 1*65536,
    7 + 4*16 + 7*256 + 1*4096 + 7*65536,
    7 + 5*16 + 7*256 + 1*4096 + 7*65536,
    4 + 4*16 + 4*256 + 4*4096 + 7*65536,
    7 + 5*16 + 7*256 + 5*4096 + 7*65536,
    7 + 4*16 + 7*256 + 5*4096 + 7*65536 );
                          
int SampleDigit(const in int n, const in vec2 vUV)
{
    //if( abs(vUV.x-0.5)>0.5 || abs(vUV.y-0.5)>0.5 ) return 0;
    vec2 q = abs(vUV-0.5);
    if( max(q.x,q.y)>0.5 ) return 0;
    

    ivec2 p = ivec2(floor(vUV * vec2(4.0, 5.0)));
    int   i = p.x + p.y*4;
    
    return (font[n]>>i) & 1;
}

int PrintInt( in vec2 uv, in int value )
{
    int res = 0;
    
    int maxDigits = (value<10) ? 1 : (value<100) ? 2 : 3;
    int digitID = maxDigits - 1 - int(floor(uv.x));
    
    if( digitID>=0 && digitID<maxDigits )
    {
        int div = (digitID==0) ? 1 : (digitID==1) ? 10 : 100;
        res = SampleDigit( (value/div) % 10, vec2(fract(uv.x), uv.y) );
    }

    return res;
}

vec4 loadValue( in ivec2 re )
{
    return texelFetch( iChannel0, re, 0 );
}

//============================================================

vec3 drawMap( vec3 col, in vec2 fragCoord )
{
    vec2 p = fragCoord/iResolution.y;
    p.x += 0.5*(1.0-iResolution.x/iResolution.y); // center
    float wp = 1.0/iResolution.y;

    vec2 q = floor(p*31.0);
    vec2 r = fract(p*31.0);
    float wr = 31.0*wp;

    if( q.x>=0.0 && q.x<=27.0 )
    {
        float c = texture( iChannel0, (q+0.5)/iResolution.xy, -100.0 ).x;

        // empty
        if( c<0.5 )
        {
        }
        // walls
        else if( c<1.5 )
        {
            vec2 wmi = vec2( texture( iChannel0, (q-vec2(1.0,0.0)+0.5)/iResolution.xy ).x,
                             texture( iChannel0, (q-vec2(0.0,1.0)+0.5)/iResolution.xy ).x );
            vec2 wma = vec2( texture( iChannel0, (q+vec2(1.0,0.0)+0.5)/iResolution.xy ).x,
                             texture( iChannel0, (q+vec2(0.0,1.0)+0.5)/iResolution.xy ).x );
			
            wmi = step( abs(wmi-1.0), vec2(0.25) );
            wma = step( abs(wma-1.0), vec2(0.25) );
            vec2 ba = -(0.16+0.35*wmi);
            vec2 bb =  (0.16+0.35*wma);

            //bb = vec2(0.51); ba = -bb;

            float d = sdBox(r-0.5, ba, bb);
            float f = 1.0 - smoothstep( -0.01, 0.01, d );
            
            vec3 wco = 0.5 + 0.5*cos( 3.9 - 0.2*(wmi.x+wmi.y+wma.x+wma.y) + vec3(0.0,1.0,1.5) );
            wco += 0.1*sin(40.0*d);
            col = mix( col, wco, f );
        }
        // points
        else if( c<2.5 )
        {
            float d = sdCircle(r-0.5, 0.15);
            float f = 1.0 - smoothstep( -wr, wr, d );
            col = mix( col, vec3(1.0,0.8,0.7), f );
            //col += 0.3*vec3(1.0,0.7,0.4)*exp(-12.0*d*d); // glow
        }
        // big alls
        else
        {
            float d = sdCircle( r-0.5 ,0.40*smoothstep( -1.0, -0.5, sin(2.0*6.2831*iTime) ));
            float f = 1.0 - smoothstep( -wr, wr, d );
            col = mix( col, vec3(1.0,0.9,0.5), f );
        }
    }
    
    return col;
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


vec3 drawPacman( vec3 col, in vec2 fragCoord, in vec4 pacmanPos, in vec3 pacmanMovDirNex )
{
    vec2 off = dir2dis(pacmanMovDirNex.x);
    
    vec2 mPacmanPos = pacmanPos.xy;
    //vec2 mPacmanPos = pacmanPos.xy + off*pacmanPos.z*pacmanPos.w;

    vec2 p = fragCoord/iResolution.y;
    float eps = 1.0 / iResolution.y;

    vec2 q = p - cell2ndc( mPacmanPos );

         if( pacmanMovDirNex.y<1.5 ) { q = q.xy*vec2(-1.0,1.0); }
    else if( pacmanMovDirNex.y<2.5 ) { q = q.xy; }
    else if( pacmanMovDirNex.y<3.5 ) { q = q.yx*vec2(-1.0,1.0); }
    else                             { q = q.yx; }

    float c = sdCircle(q, 0.023);
    float f = c;

    if( pacmanMovDirNex.y>0.5 )
    {
        float an = (0.5 + 0.5*sin(4.0*iTime*6.2831)) * 0.9;
        vec2 w = normalize( q - vec2(0.005,0.0) );

        w = vec2( w.x, abs( w.y ) );
        float m = dot( w, vec2(sin(an),cos(an)));
        f = max( f, -m );
    }
    f = 1.0 - smoothstep( -0.5*eps, 0.5*eps, f );
    col = mix( col, vec3(1.0,0.8,0.1), f );

    // glow
    //col += 0.25*vec3(1.0,0.8,0.0)*exp(-300.0*c*c);

    return col;
}

vec3 drawGhost( vec3 col, in vec2 fragCoord, in vec3 pos, in float dir, in float id, in vec3 mode )
{
    vec2 off = dir2dis(dir);

    vec2 gpos = pos.xy;

    
    vec2 p = fragCoord/iResolution.y;
    float eps = 1.0 / iResolution.y;

    vec2 q = p - cell2ndc( gpos );

    float c = sdCircle(q, 0.023);
    float f = c;
	f = max(f,-q.y);
    float on = 0.0025*sin(1.0*6.28318*q.x/0.025 + 6.2831*iTime);
    f = min( f, sdBox(q-vec2(0.0,-0.0065+on), vec2(0.023,0.012) ) );
   
    vec3 gco = 0.5 + 0.5*cos( 5.0 + 0.7*id + vec3(0.0,2.0,4.0) );
    float g = mode.x;
    if( mode.z>0.75 )
    {
        g *= smoothstep(-0.2,0.0,sin(3.0*6.28318*(iTime-mode.y)));
    }
    gco = mix( gco, vec3(0.1,0.5,1.0), g );
    
    f = 1.0 - smoothstep( -0.5*eps, 0.5*eps, f );
    col = mix( col, gco, f );

    f = sdCircle( vec2(abs(q.x-off.x*0.006)-0.011,q.y-off.y*0.006-0.008), 0.008);
    f = 1.0 - smoothstep( -0.5*eps, 0.5*eps, f );
    col = mix( col, vec3(1.0), f );

    f = sdCircle( vec2(abs(q.x-off.x*0.01)-0.011,q.y-off.y*0.01-0.008), 0.004);
    f = 1.0 - smoothstep( -0.5*eps, 0.5*eps, f );
    col = mix( col, vec3(0.0), f );

    // glow
    //col += 0.2*gco*exp(-300.0*c*c);

    return col;
}


vec3 drawScore( in vec3 col, in vec2 fragCoord, vec2 score, float lives )
{
    // score
    vec2 p = fragCoord/iResolution.y;
    col += float( PrintInt( (p - vec2(0.05,0.9))*20.0, int(score.x) ));
    col += float( PrintInt( (p - vec2(0.05,0.8))*20.0, int(242.0-score.y) ));
    
    // lives
    float eps = 1.0 / iResolution.y;
    for( int i=0; i<3; i++ )
    {
        float h = float(i);
        vec2 q = p - vec2(0.1 + 0.075*h, 0.7 );
        if( h + 0.5 < lives )
        {
            float c = sdCircle(q, 0.023);
            float f = c;

            {
                vec2 w = normalize( q - vec2(0.005,0.0) );
                w = vec2( w.x, abs( w.y ) );
                float an = 0.5;
                float m = dot( w, vec2(sin(an),cos(an)));
                f = max( f, -m );
            }
            f = 1.0 - smoothstep( -0.5*eps, 0.5*eps, f );
            col = mix( col, vec3(1.0,0.8,0.1), f );

            // glow
            //col += 0.15*vec3(1.0,0.8,0.0)*exp(-1500.0*c*c);
        }
    }

    return col;
}

//============================================================

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    //------------------------
    // load game state
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


    //------------------------
    // render
    //------------------------
    vec3 col = vec3(0.0);
    
    // map
    col = drawMap( col, fragCoord );
    
    // pacman
    col = drawPacman( col, fragCoord, pacmanPos, pacmanDir );

    // ghosts
    for( int i=0; i<4; i++ )
    {
        col = drawGhost( col, fragCoord, ghostPos[i].xyz, ghostPos[i].w, float(i), mode );
    }

    // score
    col = drawScore( col, fragCoord, points, lives );
 
    
    if( state>1.5 )
    {
        col = mix( col, vec3(0.3), smoothstep(-1.0,1.0,sin(2.0*6.2831*iTime)) );
    }
    
	fragColor = vec4( col, 1.0 );
}