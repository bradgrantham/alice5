////////////////////////////////////////////////////////////////////////////////
//"Cumulus, the flirt" - Shader Showdown practice session 012

// WHAT THE FUCK IS THE SHADER SHOWDOWN?
// The "Shader Showdown" is a demoscene live-coding shader battle competition.
// 2 coders battle for 25 minutes making a shader from memory on stage. 
// The audience votes for the winner by making noise or by voting on their phone.
// Winner goes through to the next round until the final where champion is crowned.
// Live coding shader software used is BONZOMATIC made by Gargaj from Conspiracy:
// https://github.com/Gargaj/Bonzomatic

// Every tuesdays around 21:00 UK time I practise live on TWITCH. This is the result of session 012.

// COME SEE LIVE CODING EVERY TUESDAYS HERE: https://www.twitch.tv/evvvvil_

// evvvvil / DESiRE demogroup

// "Make your wine and cheese party more interesting by having no cheese and no guests" - VIZ

vec2 sc,y,e=vec2(.000035,-.000035);float t,tt,b,de,cr=1.;vec3 np,bp,cp,po;vec4 cc,su=vec4(0);//Some fucking globals, about as exciting as hippies lecturing you about enlightenment
float bo(vec3 p,vec3 r){vec3 q=abs(p)-r;return max(max(q.x,q.y),q.z);}//box function stolen from UNC, because "cdak" is still relevent today
mat2 r2(float r){return mat2(cos(r),sin(r),-sin(r),cos(r));}//Simple rotate function, it is useful as fuck and short. Bit like victorian pickpocketing street urchins
vec2 fb(vec3 p)//Fucking bits function which makes the fucking bit/piece it is a base shape which we clone and repeate to create the whole geometry in mp function
{
  vec2 h,g=vec2(bo(abs(p)-vec3(4.3,0,0),vec3(3.5,0.4,0.4)),3);//Symetry clone couple of boxes
  h=vec2(bo(abs(p)-vec3(4.,0,0),vec3(3.,0.2,0.6)),5);//and again symetry cloning like it's 1999 and the pills were still good
  h.x=min(bo(abs(p)-vec3(0,0,1.),vec3(30,0.2,0.2)),h.x);//yeah these pills are still rushing bruv
  h.x=min(bo(p,vec3(30,0.2,0.2)),h.x);//slow down now it's 6am and im trying compare snooker to international geo politics, it's been a good night
  h.x=min(bo(abs(p)-vec3(6,0,0),vec3(0.2,100.,0.2)),h.x);//Just when you thought the rush was over your mate walks in with DMT from Mexico, ok let me grab my sombrero, at 10am you better keep it stealthy
  g=(g.x<h.x)?g:h;//Blending two shapes while retinaing material ID, at 11:00 and after all this the colours are certainly merging, but at least they retain material id in the code
  g.x*=0.7;//More definition to avoid artifact. I like my shaders looking sexy, like my wife. Don't call me shallow broh, you would be shallow too if you were handsome.
  return g;
}
float noise(vec3 p){//Noise function stolen from Virgil who stole it from Shane who I assume understands this shit, unlike me who is too busy contemplating if this cloud is indeed shaped like a penis
  vec3 ip=floor(p),s=vec3(7,157,113);
  p-=ip; vec4 h=vec4(0,s.yz,s.y+s.z)+dot(ip,s);
  p=p*p*(3.-2.*p);
  h=mix(fract(sin(h)*43758.5),fract(sin(h+s.x)*43758.5),p.x);
  h.xy=mix(h.xz,h.yw,p.y);
  return mix(h.x,h.y,p.z);
}
float c_noise(vec3 p)//Yeah i know this whole function could be inlined below in main and size coded into couple of lines with a for loop, I couldn't be fucked though, so just pasted line few times. Forgive me I was drunk
{
    float f = 0.0;
    p = p + vec3(0.0,1.0,0.0)*tt*0.3;    
    f += 0.5*noise(p);p=2.1*p;f+=0.25*noise(p+1.);p=2.2*p;
    f += 0.125*noise(p+2.); p=2.3*p;f+=0.0625*noise(p);
    return f;
}
vec2 mp( vec3 p ) //This is the main MAP function where all geometry is made/defined. It's centre stage broski, bit like your drunk uncle who stole your fun present at your 8th birthday
{//Technique is to make a new position np and tweak it, clone it, rotate it and then pass np to fb to create complex geometry from simple piece
  p*=3.0;//scale all this shit down
  np=p;//new position is set to original position
  for(int i=0;i<5;i++){//In the loop we push, rotate np into more complex "position"
    np=abs(np)-vec3(3.+cos(tt)*2.,5.-b*2.,1.-b*0.7);//symetry clone the fucker out
    np.xz*=r2(.3-b);//rotate the bitch along xz axis
    np.xy*=r2(.3-b*.5);//and again rotate the whole shit along xy so it looks symetrical
  }
  vec2 h,g=fb(np);//push np to fucking bit function to make complex geometry based on single piece, it's like punching a fish and getting a wish
  bp=np;//we make one more new position, called "bp" for the inner bits
  float att=(length(p)-1.5);//create a geometry reverse attractor like spherical field that pushes shit away from its centre broski
  bp=abs(np*.6-2.0)-vec3(2.-att*.5,-1.5+att*.7,2.5);//Clone the fucking pos one more time, i don't know, I was thinking about a large portion of fries at the time
  bp.yz*=r2(sin(tt));//keep it busy and rotatey
  bp.xz*=r2(0.1*sin(tt));// and once again we screw around with rotation for anim effect, classic fucking shit bro
  h=fb(bp);//Ja, make yet another bunch of geometry using fb piece this time with bp.
  g=(g.x<h.x)?g:h;//Mix all that geometry shit up into one fucking variable retaining material ID
  h=vec2(length(abs(bp)-2.8)-2.5,6);//Ah yeah that the inner grey geometry and it uses sphere primitive function and bp again but cloned out one more time
  h.x*=0.6;//avoid artefact by incresing definiton of distance field, you know the fucking score brovhskhi
  g=(g.x<h.x)?g:h;//This fucking merger again, colourful handshake and all that shit
  g.x*=0.4;//position was scaled hey, so tweak it back to avoid artifact
  return g;
}
vec2 tr( vec3 ro, vec3 rd )
{
  vec2 h,t=vec2(0.1);//Near plane because we all started life crying and it's important not finish that way as well
  for(int i=0;i<128;i++){//Main loop de loop 
    h=mp(ro+rd*t.x);//Marching forward like any good self-righteous activist, without care for other opinions
    if(h.x<.0001||t.x>60.) break;//Don't let the bastards break you down!
    t.x+=h.x;t.y=h.y;//Remember the postion and the material id? Yeah let's pass it on but keep the spliff, let's do this romantic thing wasted
  }
  return t;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = vec2(gl_FragCoord.x / iResolution.x, gl_FragCoord.y / iResolution.y);
    uv -= 0.5;uv /= vec2(iResolution.y / iResolution.x, 1);//boilerplate code to get uvs in BONZOMATIC live coding software i use.
    tt=mod(iTime*.5,50.);//MAin time variable, it's modulo'ed to avoid ugly artifact. Holding time in my hand: playing god is nearly as good as this crystal meth bag
  	b=0.5+clamp(sin(tt),-0.5,0.5);//This is just animation variables used in mp or fb
    vec3 ro=vec3(cos(tt*0.2)*3.,cos(tt*0.1)*2.,sin(t*0.3)*4.-10.),//Ro=ray origin=camera position because everything is relative to a view point, even your ex girlfriend's dubious taste in men
    cw=normalize(vec3(0)-ro),cu=normalize(cross(cw,vec3(0,1,0))),cv=normalize(cross(cu,cw)),
    rd=mat3(cu,cv,cw)*normalize(vec3(uv,.5)),//rd=ray direction (where the camera is pointing), co=final color, fo=fog color
    co,fo,ld=normalize(vec3(.2,-.2,-.5));//ld=light direction,
    co=fo=vec3(0.3,.4,.5)-rd.y*0.4;//By default the color fog color and it's dumb as fuck sky because Tottenham FC current Premiership form is really making me chuckle
    sc=tr(ro,rd);t=sc.x;//This is where we shoot the fucking rays to get the fucking scene. Like a soldier but with a pixel gun and less intentions to invade and pillage.    
    if(t>0.){//If t>0 then we must have hit some geometry so let's fucking shade it. 
        //We hit some geometry so let's get the current position (po) and build some normals (no). You do the Maths while I finish this barking contest with the neighbour's dog
        po=ro+rd*t;vec3 no=normalize(e.xyy*mp(po+e.xyy).x+e.yyx*mp(po+e.yyx).x+e.yxy*mp(po+e.yxy).x+e.xxx*mp(po+e.xxx).x),
        //LIGHTING MICRO ENGINE BROSKI 
        al=sc.y==5.?vec3(1,.5,0):vec3(sc.y<5.?0.:0.5);//Albedo is base colour. Change colour depending on material id, it's like art school but without the middle class suburban boredom
        float dif=max(0.,dot(no,ld)),//dif=diffuse because i ain't got time to cook torrance
        aor=t/50.,ao=exp2(-2.*pow(max(0.,1.-mp(po+no*aor).x/aor),2.)),//aor =amibent occlusion range, ao = ambient occlusion
        fr=pow(1.+dot(no,rd),4.),//Fr=fresnel which adds reflections on edges to composite geometry better, yeah could be reflected, but who gives a shit? Anyways just like you on a football b pitch: it doesn't do much.
        spo= exp2(1.0+3.0*noise(np/vec3(0.2,0.4,0.4)));//TRICK making a gloss map from a 3d noise function is a thing of fucking beauty        
        if(sc.y>5.) spo= exp2(1.0+3.0*noise(bp/vec3(0.2,0.4,0.4)));//Some geometry use bp instead of np so we need to rebuild the gloss map so it doesn't slide
        vec3 sss=vec3(0.5)*smoothstep(0.,1.,mp(po+ld*0.4).x/0.4),//sss=subsurface scatterring made by tekf from the wax shader, big up tekf! https://www.shadertoy.com/view/lslXRj
        sp=vec3(0.5)*pow(max(dot(reflect(-ld,no),-rd),0.),spo);//Sp=specualr, sotlen from Shane and it's better than bumping into your old teacher and having to explain your cocaine addiction
        co=mix(sp+al*(.8*ao+0.2)*(dif+sss),fo,min(fr,0.5));//Building the final lighting result, compressing the fuck outta everything above into an RGB shit sandwich
        co=mix(co,fo,1.-exp(-.0001*t*t*t));//Fog soften things, but it won't save you from making a fool of yourself if you comment on this shader
    }
    for (int i=0;i<60;i++) {//VOLUMETRIC CLOUDS RENDERING. 
    	cp=ro+rd*(cr+=(0.3-length(cp-ro)*0.01));//This could be fixed step cr+=0.3 or the opposite like this:cr+=(0.1+length(cp-ro)*0.01) if you want the front clouds to be more defined rather than back clouds
        cp.xz*=r2(sin(cp.y*0.1+tt)*.5);//The point of this is to be size coded to be used in 4kb intros and shader showdowns
        if (su.a>0.99||cr>t) break;//so I don't really care how it should be done, unless it's fucking size-coded broh
        de=clamp(-1.5+2.8*c_noise(cp*.5),0.,1.);//Yeah shadertoy is cute but it only counts if you compile it into a 4kb intro.
        cc=vec4(mix(vec3(1),co,de*.9)*de,de);//"4096 bytes ought to be enough for anyone"
        su+=cc*(1.-su.a);//forgot to say line 114 can be removed to make it smaller it just give clouds a little rotation animation
    } su=clamp(su,0.,1.);
	//So after speaking with LJ he came up with this to improve volumetric rendering and banding, it's not perfect and I might improve later, maybe some of clamp above is fighting with it
	//Please Don't comment telling me how i could improve with hash function, I know but I am only interested in size coded tricks. I'm happy with the trade off and not everything has to be perfect.
	fragColor=vec4(pow(mix(su.xyz,co,(1.-su.r)*smoothstep(-1.,1.,length(cp-po))),vec3(.45)),1);
	//fragColor=vec4(pow(mix(co,su.xyz,su.a),vec3(.45)),1); //This would be the simpler more naive way for doing it, without the trick from LJ
}
