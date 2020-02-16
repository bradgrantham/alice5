// Created by inigo quilez - iq/2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0


// game play

#define _ 0 // empty
#define W 1 // wall
#define P 2 // point
#define B 3 // ball
#define PA(a,b,c,d,e,f,g) (a+4*(b+4*(c+4*(d+4*(e+4*(f+4*(g)))))))
#define DD(id,c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13) if(y==id) m=(x<7)?PA(c0,c1,c2,c3,c4,c5,c6):PA(c7,c8,c9,c10,c11,c12,c13);
int map( in ivec2 q ) 
{
    if( q.x>13 ) q.x = q.x = 26-q.x;
	int x = q.x;
	int y = q.y;
	int m = 0;
    DD(30, W,W,W,W,W,W,W,W,W,W,W,W,W,W)
    DD(29, W,P,P,P,P,P,P,P,P,P,P,P,P,W)
    DD(28, W,P,W,W,W,W,P,W,W,W,W,W,P,W)
    DD(27, W,B,W,W,W,W,P,W,W,W,W,W,P,W)
    DD(26, W,P,W,W,W,W,P,W,W,W,W,W,P,W)
    DD(25, W,P,P,P,P,P,P,P,P,P,P,P,P,P)
    DD(24, W,P,W,W,W,W,P,W,W,P,W,W,W,W)
    DD(23, W,P,W,W,W,W,P,W,W,P,W,W,W,W)
    DD(22, W,P,P,P,P,P,P,W,W,P,P,P,P,W)
    DD(21, W,W,W,W,W,W,P,W,W,W,W,W,_,W)
    DD(20, _,_,_,_,_,W,P,W,W,W,W,W,_,W)
    DD(19, _,_,_,_,_,W,P,W,W,_,_,_,_,_)
    DD(18, _,_,_,_,_,W,P,W,W,_,W,W,W,_)
    DD(17, W,W,W,W,W,W,P,W,W,_,W,_,_,_)
    DD(16, _,_,_,_,_,_,P,_,_,_,W,_,_,_)
    DD(15, W,W,W,W,W,W,P,W,W,_,W,_,_,_)
    DD(14, _,_,_,_,_,W,P,W,W,_,W,W,W,W)
    DD(13, _,_,_,_,_,W,P,W,W,_,_,_,_,_)
    DD(12, _,_,_,_,_,W,P,W,W,_,W,W,W,W)
    DD(11, W,W,W,W,W,W,P,W,W,_,W,W,W,W)
    DD(10, W,P,P,P,P,P,P,P,P,P,P,P,P,W)
    DD( 9, W,P,W,W,W,W,P,W,W,W,W,W,P,W)
    DD( 8, W,P,W,W,W,W,P,W,W,W,W,W,P,W)
    DD( 7, W,B,P,P,W,W,P,P,P,P,P,P,P,_)
    DD( 6, W,W,W,P,W,W,P,W,W,P,W,W,W,W)
    DD( 5, W,W,W,P,W,W,P,W,W,P,W,W,W,W)
    DD( 4, W,P,P,P,P,P,P,W,W,P,P,P,P,W)
    DD( 3, W,P,W,W,W,W,W,W,W,W,W,W,P,W)
    DD( 2, W,P,W,W,W,W,W,W,W,W,W,W,P,W)
    DD( 1, W,P,P,P,P,P,P,P,P,P,P,P,P,P)
    DD( 0, W,W,W,W,W,W,W,W,W,W,W,W,W,W)
	return (m>>(2*(x%7))) & 3;
}

//----------------------------------------------------------------------------------------------

const int KEY_SPACE = 32;
const int KEY_LEFT  = 37;
const int KEY_UP    = 38;
const int KEY_RIGHT = 39;
const int KEY_DOWN  = 40;

const float speedPacman = 7.0;
const float speedGhost  = 6.0;
const float intelligence = 0.53;
const float modeTime = 5.0;
//----------------------------------------------------------------------------------------------

float hash(float seed)
{
    return fract(sin(seed)*158.5453 );
}

//----------------------------------------------------------------------------------------------

vec4 loadValue( in ivec2 re )
{
    return texelFetch( iChannel0, re, 0 );
}

void storeValue( in ivec2 re, in vec4 va, inout vec4 fragColor, in ivec2 fragCoord )
{
    fragColor = ( re.x==fragCoord.x && re.y==fragCoord.y ) ? va : fragColor;
}

void storeValue( in ivec4 re, in vec4 va, inout vec4 fragColor, in ivec2 fragCoord )
{
    vec2 r = 0.5*vec2(re.zw);
    vec2 d = abs( vec2(fragCoord-re.xy)-r) - r - 0.5;
    fragColor = ( -max(d.x,d.y) > 0.0 ) ? va : fragColor;
}

ivec2 dir2dis( in int dir )
{
    ivec2 off = ivec2(0,0);
         if( dir==0 ) { off = ivec2( 0, 0); }
    else if( dir==1 ) { off = ivec2( 1, 0); }
    else if( dir==2 ) { off = ivec2(-1, 0); }
    else if( dir==3 ) { off = ivec2( 0, 1); }
    else              { off = ivec2( 0,-1); }
    return off;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 ifragCoord = ivec2( fragCoord-0.5 );

    // don't compute gameplay outside of the data area
    if( ifragCoord.x > 31 || ifragCoord.y>31 ) discard;
    
    //---------------------------------------------------------------------------------   
	// load game state
	//---------------------------------------------------------------------------------
    vec4  ghostPos[4];
    vec4  pacmanPos       = loadValue( txPacmanPos );
    vec3  pacmanMovDirNex = loadValue( txPacmanMovDirNex ).xyz;
    vec2  points          = loadValue( txPoints ).xy;
    float state           = loadValue( txState ).x; // -1 = start game, 0 = start life, 1 = playing, 2 = game over
    vec3  mode            = loadValue( txMode ).xyz;
    float lives           = loadValue( txLives ).x;
    int   cell            = int( loadValue( ifragCoord ).x );
    ghostPos[0]           = loadValue( txGhost0PosDir );
    ghostPos[1]           = loadValue( txGhost1PosDir );
    ghostPos[2]           = loadValue( txGhost2PosDir );
    ghostPos[3]           = loadValue( txGhost3PosDir );
	
    //---------------------------------------------------------------------------------
    // reset
	//---------------------------------------------------------------------------------
	if( iFrame==0 ) state = -1.0;

    if( state<0.5 )
    {
        pacmanPos       = vec4(13.0,13.0,0.0,0.0);
        pacmanMovDirNex = vec3(0.0,0.0,0.0);
        mode            = vec3(0.0,-100.0,0.0);
        ghostPos[0]     = vec4(13.0,19.0,0.0,1.0);
        ghostPos[1]     = vec4(13.0,17.0,0.0,1.0);
        ghostPos[2]     = vec4(12.0,16.0,0.0,1.0);
        ghostPos[3]     = vec4(14.0,15.0,0.0,1.0);
    }
    
    if( state < -0.5 )
    {
        state           = 0.0;
        points          = vec2(0.0,0.0);
        lives           = 3.0;
        if( ifragCoord.x<27 && ifragCoord.y<31 ) 
            cell = map( ifragCoord );
    }
    else if( state < 0.5 )
    {
        state = 1.0;
    }
    else if( state < 1.5 ) 
	{
        //-------------------
        // pacman
        //-------------------

        // move with keyboard
        if( texelFetch( iChannel1, ivec2(KEY_RIGHT,0), 0 ).x>0.5 ) pacmanMovDirNex.z = 1.0;
        if( texelFetch( iChannel1, ivec2(KEY_LEFT, 0), 0 ).x>0.5 ) pacmanMovDirNex.z = 2.0;
        if( texelFetch( iChannel1, ivec2(KEY_UP,   0), 0 ).x>0.5 ) pacmanMovDirNex.z = 3.0;
        if( texelFetch( iChannel1, ivec2(KEY_DOWN, 0), 0 ).x>0.5 ) pacmanMovDirNex.z = 4.0;

        // execute desired turn as soon as possible
        if( pacmanMovDirNex.z>0.5 && abs(loadValue( ivec2(pacmanPos.xy) + dir2dis(int(pacmanMovDirNex.z)) ).x-float(W))>0.25 )
        {
            pacmanMovDirNex = vec3( pacmanMovDirNex.zz, 0.0 );
        }
        
        
        if( pacmanMovDirNex.x>0.5 ) pacmanPos.z += iTimeDelta*speedPacman;

        ivec2 off = dir2dis(int(pacmanMovDirNex.x));
        ivec2 np = ivec2(pacmanPos.xy) + off;
        float c = loadValue( np ).x;
        pacmanPos.w = step( 0.25, abs(c-float(W)) );

                
        if( pacmanPos.z>=1.0 )
        {
            pacmanPos.z = 0.0;
            float c = loadValue( np ).x;

            if( abs(c-float(W))<0.25 )
            {
                pacmanMovDirNex.x = 0.0;
            }
            else
            {
                pacmanPos.xy += vec2(off);
                // tunnel!
                     if( pacmanPos.x< 0.0 ) pacmanPos.x=26.0;
                else if( pacmanPos.x>26.0 ) pacmanPos.x= 0.0;
            }

            bool isin = (ifragCoord.x==int(pacmanPos.x)) && (ifragCoord.y==int(pacmanPos.y));
            c = loadValue( ivec2(pacmanPos.xy) ).x;
            if( abs(c-float(P))<0.2 )
            {
                if( isin ) cell = _;
                points += vec2(10.0,1.0);
            }
            else if( abs(c-float(B))<0.2 )
            {
                if( isin ) cell = _;
                points += vec2(50.0,1.0);
                mode.x = 1.0;
                mode.y = iTime;
            }
            if( points.y>241.5 )
            {
                state = 2.0;
            }
        }
        
        //-------------------
        // ghost
        //-------------------

        for( int i=0; i<4; i++ )
        {
            float seed = float(iFrame)*13.1 + float(i)*17.43;

            ghostPos[i].z += iTimeDelta*speedGhost;

            if( ghostPos[i].z>=1.0 )
            {
                ghostPos[i].z = 0.0;

                float c = loadValue( ivec2(ghostPos[i].xy)+dir2dis(int(ghostPos[i].w)) ).x;

                bool wr = int(loadValue( ivec2(ghostPos[i].xy)+ivec2( 1, 0) ).x) == W;
                bool wl = int(loadValue( ivec2(ghostPos[i].xy)+ivec2(-1, 0) ).x) == W;
                bool wu = int(loadValue( ivec2(ghostPos[i].xy)+ivec2( 0, 1) ).x) == W;
                bool wd = int(loadValue( ivec2(ghostPos[i].xy)+ivec2( 0,-1) ).x) == W;

                vec2 ra = vec2( hash( seed + 0.0),
                                hash( seed + 11.57) );
                if( abs(c-float(W)) < 0.25) // found a wall on the way
                {
                    if( ghostPos[i].w < 2.5 ) // was moving horizontally
                    {
                             if( !wu &&  wd )                ghostPos[i].w = 3.0;
                        else if(  wu && !wd )                ghostPos[i].w = 4.0;
                        else if( pacmanPos.y>ghostPos[i].y ) ghostPos[i].w = 3.0+mode.x;
                        else if( pacmanPos.y<ghostPos[i].y ) ghostPos[i].w = 4.0-mode.x;
                        else                                 ghostPos[i].w = 3.0-ghostPos[i].w;
                    }
                    else                          // was moving vertically
                    {
                             if( !wr &&  wl )                ghostPos[i].w = 1.0;
                        else if(  wr && !wl )                ghostPos[i].w = 2.0;
                        else if( pacmanPos.x>ghostPos[i].x ) ghostPos[i].w = 1.0+mode.x;
                        else if( pacmanPos.x<ghostPos[i].x ) ghostPos[i].w = 2.0-mode.x;
                        else                                 ghostPos[i].w = 7.0-ghostPos[i].w;
                    }

                }
                else if( ra.x < intelligence ) // found an intersection and it decided to find packman
                {
                    if( ghostPos[i].w < 2.5 ) // was moving horizontally
                    {
                             if( !wu && pacmanPos.y>ghostPos[i].y ) ghostPos[i].w = 3.0;
                        else if( !wd && pacmanPos.y<ghostPos[i].y ) ghostPos[i].w = 4.0;
                    }
                    else                          // was moving vertically
                    {
                             if( !wr && pacmanPos.x>ghostPos[i].x ) ghostPos[i].w = 1.0;
                        else if( !wl && pacmanPos.x<ghostPos[i].x ) ghostPos[i].w = 2.0;
                    }
                }
                else
                {
                         if( ra.y<0.15 ) { if( !wr ) ghostPos[i].w = 1.0; }
                    else if( ra.y<0.30 ) { if( !wl ) ghostPos[i].w = 2.0; }
                    else if( ra.y<0.45 ) { if( !wu ) ghostPos[i].w = 3.0; }
                    else if( ra.y<0.60 ) { if( !wd ) ghostPos[i].w = 4.0; }
                }

                if( abs(ghostPos[i].x-13.0)<0.25 &&
                    abs(ghostPos[i].y-19.0)<0.25 && 
                    abs(ghostPos[i].w-4.0)<0.25 )
                {
                    ghostPos[i].w = 1.0;
                }
                
                ghostPos[i].xy += vec2(dir2dis(int(ghostPos[i].w)));
                    
                    // tunnel!
                     if( ghostPos[i].x< 0.0 ) ghostPos[i].x=26.0;
                else if( ghostPos[i].x>26.0 ) ghostPos[i].x= 0.0;
            }
            
            
            // collision
            if( abs(pacmanPos.x-ghostPos[i].x)<0.5 && abs(pacmanPos.y-ghostPos[i].y)<0.5 )
            {
                if( mode.x<0.5 )
                {
                    lives -= 1.0;
                    if( lives<0.5 )
                    {
                		state = 2.0;
                    }
                    else
                    {
                        state = 0.0;
                    }
                }
                else
                {
                    points.x += 200.0;
                    ghostPos[i] = vec4(13.0,19.0,0.0,1.0);
                }
            }
        }
 
        //-------------------
        // mode
        //-------------------
        mode.z = (iTime-mode.y)/modeTime;
        if( mode.x>0.5 && mode.z>1.0 )
        {
            mode.x = 0.0;
        }
    }
    else //if( state > 0.5 )
    {
        float pressSpace = texelFetch( iChannel1, ivec2(KEY_SPACE,0), 0 ).x;
        if( pressSpace>0.5 )
        {
            state = -1.0;
        }
    }
  
	//---------------------------------------------------------------------------------
	// store game state
	//---------------------------------------------------------------------------------
    fragColor = vec4(0.0);
 
    
    storeValue( txPacmanPos,        vec4(pacmanPos),             fragColor, ifragCoord );
    storeValue( txPacmanMovDirNex,  vec4(pacmanMovDirNex,0.0),   fragColor, ifragCoord );
    storeValue( txGhost0PosDir,     vec4(ghostPos[0]),           fragColor, ifragCoord );
    storeValue( txGhost1PosDir,     vec4(ghostPos[1]),           fragColor, ifragCoord );
    storeValue( txGhost2PosDir,     vec4(ghostPos[2]),           fragColor, ifragCoord );
    storeValue( txGhost3PosDir,     vec4(ghostPos[3]),           fragColor, ifragCoord );
    storeValue( txPoints,           vec4(points,0.0,0.0),        fragColor, ifragCoord );
    storeValue( txState,            vec4(state,0.0,0.0,0.0),     fragColor, ifragCoord );
    storeValue( txMode,             vec4(mode,0.0),              fragColor, ifragCoord );
    storeValue( txLives,            vec4(lives,0.0,0.0,0.0),     fragColor, ifragCoord );
    storeValue( txCells,            vec4(cell,0.0,0.0,0.0),      fragColor, ifragCoord );
}