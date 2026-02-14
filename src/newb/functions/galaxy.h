#include "detection.h"
#include "noise.h"

vec3 nlGalaxy(vec3 vdir, vec3 fogColor, nl_environment env, float t) {
  vec3 stars;
  if (env.underwater) {
    stars =  vec3_splat(0.0);
  }

  t *= NL_GALAXY_SPEED;

  // rotate space
  float cosb = sin(0.2*t);
  float sinb = cos(0.2*t);
  vdir.xy = mul(mat2(cosb, sinb, -sinb, cosb), vdir.xy);

  // noise
  float n0 = 0.5 + 0.5*sin(5.0*vdir.x)*sin(5.0*vdir.y - 0.5*t)*sin(5.0*vdir.z + 0.5*t);
  float n1 = noise3D(15.0*vdir + sin(0.85*t + 1.3));
  float n2 = noise3D(50.0*vdir + 1.0*n1 + sin(0.7*t + 1.0));
  float n3 = noise3D(200.0*vdir - 10.0*sin(0.4*t + 0.500));

  // stars
  n3 = smoothstep(0.04,0.3,n3+0.02*n2);
  float gd = vdir.x + 0.1*vdir.y + 0.1*sin(10.0*vdir.z + 0.2*t);
  float st = n1*n2*n3*n3*(1.0+70.0*gd*gd);
  st = (1.0-st)/(1.0+400.0*st);
  stars = vec3(0.28, 0.50, 1.3) * st;

  /*
  // glow
  float gfmask = abs(vdir.x)-0.15*n1+0.04*n2+0.25*n0;
  float gf = 1.0 - (vdir.x*vdir.x + 0.03*n1 + 0.2*n0);
  gf *= gf;
  gf *= gf*gf;
  gf *= 1.0-0.3*smoothstep(0.2, 0.3, gfmask);
  gf *= 1.0-0.2*smoothstep(0.3, 0.4, gfmask);
  gf *= 1.0-0.1*smoothstep(0.2, 0.1, gfmask);
  vec3 gfcol = normalize(vec3(n0, cos(2.0*vdir.y), sin(vdir.x+n0)));
  stars += (0.4*gf + 0.012)*mix(vec3(0.5, 0.5, 0.5), gfcol*gfcol, NL_GALAXY_VIBRANCE);
  */
  stars *= mix(1.0, NL_GALAXY_DAY_VISIBILITY, min(dot(fogColor, vec3(0.5,0.7,0.5)), 1.0)); // maybe add day factor to env for global use?

  return stars*(1.0-env.rainFactor);
}