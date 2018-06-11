// The "map" function for our fractal
// Arguments:
// `p`: The point to sample
// Return:
// `resColor`:  the color to render at this sample
// `out`:       the "magnitude" at this point
float map( in vec3 p, out vec4 resColor )
{
  // experimental high-precision map function
  vec3 w = p;
  float m = dot(w,w);

  vec4 trap = vec4(abs(w),m);
  float dz = startOffset;

  for( int i=0; i<mapIterCount; i++ )
  {
    // +y: Real axis
    // +z: i
    // +x: j
    // j**2 = -1
    // i**2 = -1
    
    // this expansion of the above for modulus = 8
    // provided by inigo quilez
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
    trap = min( trap, vec4(abs(w), m) );

    m = dot(w,w);
    if( m > 64*escapeFactor )
      break;
  }

  resColor = vec4(m,trap.yzw);

  return mapResultFactor*0.25*log(float(m))*sqrt(m)/dz;
}
