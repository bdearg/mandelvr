#version 430 core
precision highp float;

#define AA 1
//#define STEPLENGTH .25
#define STEPLENGTH .25
#define STEPCOUNT 128
//#define STEPCOUNT 128

// skeleton of shader by inigo quilez

uniform vec3 clearColor;

uniform vec3 yColor;
uniform vec3 zColor;
uniform vec3 wColor;
uniform vec3 diffc1;
uniform vec3 diffc2;
uniform vec3 diffc3;

uniform vec2 resolution;

uniform float zoomLevel;
uniform float startOffset;

layout(r32f) uniform restrict readonly image2D inputDepthBuffer;
layout(r32f) uniform restrict writeonly image2D outputDepthBuffer;

uniform vec3 camOrigin;
// camera transformation
uniform mat4 view;

uniform int intersectStartStep;

uniform float intersectThreshold;
uniform int intersectStepCount;
uniform float intersectStepFactor;

uniform int modulo;

uniform float fle;

uniform int mapIterCount;
uniform float juliaFactor;
uniform vec3 juliaPoint;

uniform float time;

uniform bool movingJulia;
uniform bool exhaust;
uniform bool doFog;

out vec4 color;

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

struct expanded_float {
  int exponent;
  int mantissa;
};

// a culling bounds detector for a sphere
// Arguments:
// `sph.xyz`: Center of the sphere
// `sph.w`:   Radius of the sphere
// `ro`:      Origin of the ray
// `rd`:      Forward axis of the ray
// Return:
// `out.x`:   Near intercept along the `rd` axis. -1. on a miss
// `out.y`:   Far intercept along the `rd` axis. -1. on a miss
vec2 isphere( in vec4 sph, in vec3 ro, in vec3 rd )
{
  vec3 oc = ro - sph.xyz;

  float b = dot(oc,rd);
  float c = dot(oc,oc) - sph.w*sph.w;
  float h = b*b - c;

  if( h<0.0) return vec2(-1.0);

  h = sqrt( h );

  return -b + vec2(-h,h);
}

// The "map" function for our fractal
// Arguments:
// `p`: The point to sample
// Return:
// `resColor`:  the color to render at this sample
// `out`:       the "magnitude" at this point
float map(in int mapsteps, in vec3 p, out vec4 resColor )
{

  vec3 w = p;
  float m = dot(w,w);

  vec4 trap = vec4(abs(w),m);
  float dz = startOffset;
  int maxMapIter = mapIterCount;
  maxMapIter = max(mapsteps,maxMapIter);
  
  for( int i=0; i<maxMapIter; i++ )
  {
	//julia bulb
	
	vec3 jp = juliaPoint;
	if(movingJulia)
	{
	  jp = vec3(.4*cos(.25*time+1.), .2*sin(time+.2)+.7*sin(time*.33+0.5), .7*cos(time*.33+.05));
	}
    dz = modulo*pow(sqrt(m),modulo-1.)*dz + 1.0;
    //dz = 8.0*pow(m,3.5)*dz + 1.0;

    float r = length(w);
    float b = modulo*acos( w.y/r);
    float a = modulo*atan( w.x, w.z );
    w = mix(p, jp, clamp(juliaFactor, 0., 1.)) + pow(r,modulo) * vec3( sin(b)*sin(a), cos(b), sin(b)*cos(a) );

    trap = min( trap, vec4(abs(w), m) );

    m = dot(w,w);
    if( m > modulo*modulo )
      break;
  }

  resColor = vec4(m,trap.yzw);

  return 0.25*log(m)*sqrt(m)/dz;

}

float intersect( in vec3 ro, in vec3 rd, out vec4 rescol, in float px, in ivec2 coord, out int g)
{
  float res = -1.0;

  // bounding sphere
  vec2 dis = isphere( vec4(0.0,0.0,0.0,1.25*zoomLevel), ro, rd );
  if( dis.y<0.0 )
    return -1.0;
  dis.x = max( dis.x, 0.0 );
  dis.y = min( dis.y, 10.0 );

  // raymarch fractal distance field
  vec4 trap;
  int i;

  float t = dis.x;

  for( i=0; i<intersectStepCount; i++ )
  {
    vec3 pos = (ro + rd*t)/zoomLevel; //when i==0 pos is on the surface!?!?!

    float th = intersectThreshold*px*t;

    int imp = int((4*(2 + log(zoomLevel)) - 8*t));
    g = imp;

    float h = map(imp, pos, trap );
    if( t>dis.y || h<th ) 
		break;
    t += zoomLevel*intersectStepFactor*h;
  }

  // this also trips if a ray goes parallel to an edge, causing
  // odd borders
  // hrmmph

  if ( i >= intersectStepCount && !exhaust) // Leave some for the next step
  {
    discard;
  }
  else if( t<dis.y ) // Either a hit, or enough distance traveled
  {
    rescol = trap;
    res = t;
  }

  return res;
}

float softshadow( in vec3 ro, in vec3 rd, in float k )
{
  float res = 1.0;
  float t = 0.0;
  for( int i=0; i<64; i++ )
  {
    vec4 kk;
    float h = map(mapIterCount,ro + rd*t, kk);
    res = min( res, k*h/t );
    if( res<0.001 ) break;
    t += clamp( h, 0.01, 0.2 );
  }
  return clamp( res, 0.0, 1.0 );
}

vec3 calcNormal( in int maplvl, in vec3 pos, in float t, in float px )
{
//  return vec3(1.0, 0.0, 0.0);
  vec4 tmp;
  vec2 eps = vec2( zoomLevel*0.25*px, 0.0 );
  return normalize( vec3(
        map(maplvl,pos+eps.xyy,tmp) - map(maplvl,pos-eps.xyy,tmp),
        map(maplvl,pos+eps.yxy,tmp) - map(maplvl,pos-eps.yxy,tmp),
        map(maplvl,pos+eps.yyx,tmp) - map(maplvl,pos-eps.yyx,tmp) ) );

}

const vec3 light1 = vec3(  0.577, 0.577, -0.577 );
const vec3 light2 = vec3( -0.707, 0.000,  0.707 );


vec3 render( in vec2 p, in mat4 cam )
{
  // ray setup
  // this is our distance from the bulb

  // sp: scaled point - Converting from:
  // (0, 0) -> resolution.xy
  // to
  // (-1, -1) -> (1, 1)
  float smallestaxis = min(resolution.x, resolution.y);
  vec2  sp = (-resolution.xy + 2.0*p) / smallestaxis;
  float px = 2.0/(smallestaxis*fle);

  // extract translation component of view matrix
  vec3  ro = camOrigin;
  // extract direction from view matrix and given pixel to be marched
  vec3  rd = normalize( (cam*vec4(sp,fle,0.0)).xyz );

  // intersect fractal
  vec4 tra;
  // rounded to integer
  ivec2 ip = ivec2(p);
  int g=0;
  float t = intersect( ro, rd, tra, px, ip,g );

  vec3 col;

  vec3 skycol = clearColor*(0.6+0.4*rd.y);
  skycol += 5.0*clearColor*pow( clamp(dot(rd,light1),0.0,1.0), 32.0 );

  if( t<0.0 )
  {
//    col  = vec3(0.8,.95,1.0)*(0.6+0.4*rd.y);
//    col += 5.0*vec3(0.8,0.7,0.5)*pow( clamp(dot(rd,light1),0.0,1.0), 32.0 );
    col  = skycol;
  }
  // color fractal
  else
  {
    // color
    col = vec3(0.01);
    col = mix( col, yColor, clamp(tra.y,0.0,1.0) );
    col = mix( col, zColor, clamp(tra.z*tra.z,0.0,1.0) );
    col = mix( col, wColor, clamp(pow(tra.w,6.0),0.0,1.0) );
    col *= 0.5;
    //col = vec3(0.1);

    // lighting terms
    vec3 pos = (ro + t*rd)/zoomLevel;
    vec3 nor = calcNormal( g, pos, t, px );
    vec3 hal = normalize( light1-rd);
    vec3 ref = reflect( rd, nor );
    float occ = clamp(0.05*log(tra.x),0.0,1.0);
    float fac = clamp(1.0+dot(rd,nor),0.0,1.0);

    // sun
    float sha1 = softshadow( pos+0.001*nor, light1, 32.0 );
    //float sha1 = 1.0; //softshadow( pos+0.001*nor, light1, 32.0 );
    float dif1 = clamp( dot( light1, nor ), 0.0, 1.0 )*sha1;
    float spe1 = pow( clamp(dot(nor,hal),0.0,1.0), 32.0 )*dif1*(0.04+0.96*pow(clamp(1.0-dot(hal,light1),0.0,1.0),5.0));
    // bounce
    float dif2 = clamp( 0.5 + 0.5*dot( light2, nor ), 0.0, 1.0 )*occ;
    // sky
    float dif3 = (0.7+0.3*nor.y)*(0.2+0.8*occ);

    vec3 lin = vec3(0.0); 
    lin += 7.0*diffc1*dif1;
    lin += 4.0*diffc2*dif2;
    lin += 1.5*diffc3*dif3;
    lin += 2.5*vec3(0.35,0.30,0.25)*(0.05+0.95*occ); // ambient
    lin += 4.0*fac*occ;                          // fake SSS
    col *= lin;
    col = pow( col, vec3(0.7,0.9,1.0) );                  // fake SSS
    col += spe1*15.0;
    //col += 8.0*vec3(0.8,0.9,1.0)*(0.2+0.8*occ)*(0.03+0.97*pow(fac,5.0))*smoothstep(0.0,0.1,ref.y )*softshadow( pos+0.01*nor, ref, 2.0 );
    //col = vec3(occ*occ);

	if(doFog)
	{
		// add "hiding fog"
		col = mix( col, skycol, clamp(t*zoomLevel - 1./zoomLevel, 0., 1.));
	}
  }
  // gamma
  return sqrt( col );
}

void main()
{
  mat4 cam = view;
  // render
#if AA<2
  vec3 col = render(  gl_FragCoord.xy, cam );
#else
  vec3 col = vec3(0.0);
  for( int j=0; j<AA; j++ )
    for( int i=0; i<AA; i++ )
    {
      col += render( gl_FragCoord .xy+ (vec2(i,j)/float(AA)), cam );
    }
  col /= float(AA*AA);
#endif
  
  color = vec4( col, 1.0 );
}
