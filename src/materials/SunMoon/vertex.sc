$input a_position, a_texcoord0
$output v_texcoord0, v_pos

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/config.h>
  SAMPLER2D_AUTOREG(s_SunMoonTexture);
#endif

void main() {
  v_texcoord0 = a_texcoord0;
  #ifndef INSTANCING
    vec3 pos = clamp(a_position, -1.0, 1.0); //supposed to fix the stretching
    v_pos = pos;
    pos.xz = clamp(pos.xz, -0.5, 0.5);
    //moon detector
ivec2 ts = textureSize(s_SunMoonTexture, 0);
bool isMoon = ts.x > ts.y;

pos.xz *= 10.0;
pos.y *= 2.0; //supposed to make the sun further away


if (isMoon){
  pos.xz *= NL_MOON_SIZE;
}else{
  pos.xz *= NL_SUN_SIZE;
  }

    #ifdef NL_SUNMOON_ANGLE
      float angle = NL_SUNMOON_ANGLE*0.0174533;
      float sinA = sin(angle);
      float cosA = cos(angle);
      pos.xz = vec2(pos.x*cosA - pos.z*sinA, pos.x*sinA + pos.z*cosA);
    #endif
    gl_Position = mul(u_modelViewProj, vec4(pos, 1.0));
  #else
    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
