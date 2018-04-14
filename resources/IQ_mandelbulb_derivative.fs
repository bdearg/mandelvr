#version 430 core

#define AA 1

uniform mat4 view;
uniform mat4 headpose;
uniform mat4 rotationoffset;
uniform mat4 projection; // Left, Right, Top, Bottom half angle tangents for vr projection
uniform vec3 viewoffset;
uniform float viewscale;
uniform float unitIPD;

uniform vec2 resolution;
uniform float time;

uniform float intersectStepSize;
uniform int intersectStepCount;

out vec4 color;

/*

float map( in vec3 p, out vec4 resColor )
{
    vec3 w = p;
    float m = dot(w,w);

    vec4 trap = vec4(abs(w),m);
  float dz = 1.0;
    
    
  for( int i=0; i<4; i++ )
    {
#if 0
        float m2 = m*m;
        float m4 = m2*m2;
        dz = 8.0*sqrt(m4*m2*m)*dz + 1.0;

        float x = w.x; float x2 = x*x; float x4 = x2*x2;
        float y = w.y; float y2 = y*y; float y4 = y2*y2;
        float z = w.z; float z2 = z*z; float z4 = z2*z2;

        float k3 = x2 + z2;
        float k2 = inversesqrt( k3*k3*k3*k3*k3*k3*k3 );
        float k1 = x4 + y4 + z4 - 6.0*y2*z2 - 6.0*x2*y2 + 2.0*z2*x2;
        float k4 = x2 - y2 + z2;

        w.x = p.x +  64.0*x*y*z*(x2-z2)*k4*(x4-6.0*x2*z2+z4)*k1*k2;
        w.y = p.y + -16.0*y2*k3*k4*k4 + k1*k1;
        w.z = p.z +  -8.0*y*k4*(x4*x4 - 28.0*x4*x2*z2 + 70.0*x4*z4 - 28.0*x2*z2*z4 + z4*z4)*k1*k2;
#else
        dz = 8.0*pow(sqrt(m),7.0)*dz + 1.0;
    //dz = 8.0*pow(m,3.5)*dz + 1.0;
        
        float r = length(w);
        float b = 8.0*acos( w.y/r);
        float a = 8.0*atan( w.x, w.z );
        w = p + pow(r,8.0) * vec3( sin(b)*sin(a), cos(b), sin(b)*cos(a) );
#endif        
        
        trap = min( trap, vec4(abs(w),m) );

        m = dot(w,w);
    if( m > 256.0 )
            break;
    }

    resColor = vec4(m,trap.yzw);

    return 0.25*log(m)*sqrt(m)/dz;
}*/

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// this appears to be our "fractal rule"?
vec2 isphere( in vec4 sph, in vec3 ro, in vec3 rd )
{
  vec3 oc = ro - sph.xyz;

  float b = dot(oc,rd);
  float c = dot(oc,oc) - sph.w*sph.w;
  float h = b*b - c;

  if( h<0.0 ) return vec2(-1.0);

  h = sqrt( h );

  return -b + vec2(-h,h);
}

float map( in vec3 p, out vec4 resColor )
{

  vec3 w = p;
  float m = dot(w,w);

  vec4 trap = vec4(abs(w),m);
  float dz = 1.0;

  for( int i=0; i<4; i++ )
  {
    dz = 8.0*pow(sqrt(m),7.0)*dz + 1.0;
    //dz = 8.0*pow(m,3.5)*dz + 1.0;

    float r = length(w);
    float b = 8.0*acos( w.y/r);
    float a = 8.0*atan( w.x, w.z );
    w = p + pow(r,8.0) * vec3( sin(b)*sin(a), cos(b), sin(b)*cos(a) );

    trap = min( trap, vec4(abs(w),m) );

    m = dot(w,w);
    // 
    if( m > 256.0 )
      break;
  }

  resColor = vec4(m,trap.yzw);

  return 0.25*log(m)*sqrt(m)/dz;
}

float intersect( in vec3 ro, in vec3 rd, out vec4 rescol, in float px )
{
  float res = -1.0;

  // bounding sphere
  vec2 dis = isphere( vec4(0.0,0.0,0.0,1.25), ro, rd );
  if( dis.y<0.0 )
    return -1.0;
  dis.x = max( dis.x, 0.0 );
  dis.y = min( dis.y, 10.0 );

  // raymarch fractal distance field
  vec4 trap;

  float t = dis.x;
  for( int i=0; i<intersectStepCount; i++  )
  { 
    vec3 pos = ro + rd*t;
    float th = (intersectStepSize)*px*t;
    float h = map( pos, trap );
    if( t>dis.y || h<th ) break;
    t += h;
  }


  if( t<dis.y )
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
    float h = map(ro + rd*t, kk);
    res = min( res, k*h/t );
    if( res<0.001 ) break;
    t += clamp( h, 0.01, 0.2 );
  }
  return clamp( res, 0.0, 1.0 );
}

void vr_ray_projection(in vec2 clipspace, in mat4 cam, out vec3 ro, out vec3 rd){
  mat4 trcam = transpose(rotationoffset)*transpose(cam);
  vec3 cspr = vec3(clipspace.x, clipspace.y, 1.0);
  vec3 vspr = (inverse(projection)*vec4(cspr, 1.0)).xyz;

  vec3 eyeoffset = inverse(cam)[3].xyz - (headpose)[3].xyz;
  ro = viewoffset*vec3(1.0, 1.0, -1.0) + (headpose)[3].xyz*viewscale + eyeoffset*unitIPD*viewscale;
  //ro =  (headpose)[3].xyz*viewscale + eyeoffset*UNITIPD*viewscale;
  rd = trcam[0].xyz*vspr.x + trcam[1].xyz*vspr.y + trcam[2].xyz*vspr.z;
}

vec3 calcNormal( in vec3 pos, in float t, in float px )
{
  //return vec3(1.0, 0.0, 0.0);
  vec4 tmp;
  vec2 eps = vec2( 0.25*px, 0.0 );
  return normalize( vec3(
        map(pos+eps.xyy,tmp) - map(pos-eps.xyy,tmp),
        map(pos+eps.yxy,tmp) - map(pos-eps.yxy,tmp),
        map(pos+eps.yyx,tmp) - map(pos-eps.yyx,tmp) ) );

}

const vec3 light1 = vec3(  0.577, 0.577, -0.577 );
const vec3 light2 = vec3( -0.707, 0.000,  0.707 );


vec3 render( in vec2 p, in mat4 cam )
{


  // ray setup
  // this is our distance from the bulb
  const float fle = 1.5;

  // sp: scaled point - Converting from:
  // (0, 0) -> resolution.xy
  // to
  // (-res.x/res.y, -1) -> (res.x/res.y, 1)
  // vec2  sp = (-resolution.xy + 2.0*p) / resolution.y;

  vec2 clipspace = (p / resolution.y)*2.0 - 1.0;
  float px = 2.0/(resolution.y*fle);

  // Ray origin and Ray direction derived from view matrix. 
  vec3 ro, rd;
  vr_ray_projection(clipspace, cam, ro, rd);

  // return(rd);
 // vec3  rd = normalize( (cam*vec4(clipspace,fle,0.0)).xyz );

  // intersect fractal
  vec4 tra;
  float t = intersect( ro, rd, tra, px );

  vec3 col;

  // color sky
  if( t<0.0 )
  {
    const vec3 color_sky_1 = vec3(0.8,.95,1.0);
    col  = color_sky_1*(0.6+0.4*rd.y);
    col += 5.0*vec3(0.8,0.7,0.5)*pow( clamp(dot(rd,light1),0.0,1.0), 32.0 );
  }
  // color fractal
  else
  {
    // color
    col = vec3(0.1);
    col = mix( col, vec3(0.10,0.20,0.30), clamp(tra.y,0.0,1.0) );
    col = mix( col, vec3(0.02,0.10,0.30), clamp(tra.z*tra.z,0.0,1.0) );
    col = mix( col, vec3(0.30,0.10,0.02), clamp(pow(tra.w,6.0),0.0,1.0) );
    col *= 0.5;
    //col = vec3(0.1);

    // lighting terms
    vec3 pos = ro + t*rd;
    vec3 nor = calcNormal( pos, t, px );
    vec3 hal = normalize( light1-rd);
    vec3 ref = reflect( rd, nor );
    float occ = clamp(0.05*log(tra.x),0.0,1.0);
    float fac = clamp(1.0+dot(rd,nor),0.0,1.0);

    // sun
    float sha1 = 1.0;//softshadow( pos+0.001*nor, light1, 32.0 );
    //float sha1 = 1.0; //softshadow( pos+0.001*nor, light1, 32.0 );
    float dif1 = clamp( dot( light1, nor ), 0.0, 1.0 )*sha1;
    float spe1 = pow( clamp(dot(nor,hal),0.0,1.0), 32.0 )*dif1*(0.04+0.96*pow(clamp(1.0-dot(hal,light1),0.0,1.0),5.0));
    // bounce
    float dif2 = clamp( 0.5 + 0.5*dot( light2, nor ), 0.0, 1.0 )*occ;
    // sky
    float dif3 = (0.7+0.3*nor.y)*(0.2+0.8*occ);

    vec3 lin = vec3(0.0); 
    lin += 7.0*vec3(1.50,1.10,0.70)*dif1;
    lin += 4.0*vec3(0.25,0.20,0.15)*dif2;
    lin += 1.5*vec3(0.10,0.20,0.30)*dif3;
    lin += 2.5*vec3(0.35,0.30,0.25)*(0.05+0.95*occ); // ambient
    lin += 4.0*fac*occ;                          // fake SSS
    col *= lin;
    col = pow( col, vec3(0.7,0.9,1.0) );                  // fake SSS
    col += spe1*15.0;
    col += 8.0*vec3(0.8,0.9,1.0)*(0.2+0.8*occ)*(0.03+0.97*pow(fac,5.0))*smoothstep(0.0,0.1,ref.y )*softshadow( pos+0.01*nor, ref, 2.0 );
    //col = vec3(occ*occ);
  }

  // gamma
  return sqrt( col );
}

void main()
{

  float alttime = time*10.;

  // camera
  // camera distance
  float di = 1.4+0.1*cos(.29*alttime);
  // rotation???
  // this vector has 3 components that oscillate at different frequencies
  // so no guarantees of magnitude
  // it's responsible for our view of the bulb rotating
  // I think it marches "across" a composed sphere-ish thing?
  // there's probably a math word for this since it's not *actually* an ellipse, sphere, w/e
  // since due to the lack of frequency sync there's going to be more erratic behavior
  vec3  ro = di * vec3( cos(.33*alttime), 0.8*sin(.37*alttime), sin(.31*alttime) );
  //vec3  ro = di * vec3(.71, .71, 0);
  // uhhhh, a "bias" factor for the "ro" vector above?
  // not actually needed
  //vec3  ta = vec3(0.0,0.1,0.0);
  vec3  ta = vec3(0.1*cos(alttime),0.1*sin(alttime),0.0);
  // some thing to make a rotating vector in the xy plane
  // for construction of an orthonormal camera basis?
  float cr = 0.5*cos(0.1*alttime);

  // camera matrix
  
  // we're composing 3 vec3s to make a vector space
  vec3 cw = normalize(-ro);
  // cp's only use: constructing "cu"
  vec3 cp = vec3(sin(cr), cos(cr),0.0);
  vec3 cu = normalize(cross(cw,cp));
  vec3 cv = normalize(cross(cu,cw));
  /*mat4 cam = mat4(cu, ro.x,
      cv, ro.y,
      cw, ro.z, 
      0.0, 0.0, 0.0, 1.0);*/
  
  mat4 cam = view;

  // render
#if AA<2
  vec3 col = render(  gl_FragCoord.xy, cam );
#else
  vec3 col = vec3(0.0);
  for( int j=0; j<AA; j++ )
    for( int i=0; i<AA; i++ )
    {
      col += render( gl_FragCoord.xy + (vec2(i,j)/float(AA)), cam );
    }
  col /= float(AA*AA);
#endif

  color = vec4( col, 1.0 );
}
