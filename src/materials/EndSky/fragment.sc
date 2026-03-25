#ifndef INSTANCING
$input v_texcoord0, v_posTime, v_fogColor
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>

  SAMPLER2D_AUTOREG(s_SkyTexture);
#endif
#include <newb/config.h>
#include <newb/functions/starfield.h>

void main() {
  #ifndef INSTANCING
    vec3 viewDir = normalize(v_posTime.xyz);
    vec4 diffuse = texture2D(s_SkyTexture, v_texcoord0);

    nl_environment env;
    env.end = true;
    env.underwater = false;
    env.rainFactor = 0.0;
    env.nether    = false;

    vec3 color = renderEndSky(getEndHorizonCol(), getEndZenithCol(), viewDir, v_posTime.w);
    
    // starfield
    #ifdef STARFIELD 
      color += renderStarfield(viewDir, v_posTime.w) * 0.5;
    #endif

    #ifdef END_LYNX_AURORA
      vec4 aurora = rdAurora(v_posTime.xyz*0.001, viewDir, env, v_posTime.w, vec3(0.0,0.0,0.0), 0.0);
      color = mix(color, aurora.rgb*0.5, aurora.a);
    #endif 

    #ifdef NL_END_GALAXY_STARS
      color.rgb += NL_END_GALAXY_STARS*nlRenderGalaxy(viewDir, color, env, v_posTime.w);
    #endif

    #ifdef NL_END_CLOUD
      color.rgb += renderCloudsEnd(getEndHorizonCol(), getEndZenithCol(), viewDir, v_posTime.w);
    #endif

    #ifdef NL_SHOOTING_STAR
      color.rgb += NL_SHOOTING_STAR*nlRenderShootingStar(viewDir, v_fogColor, v_posTime.w);
    #endif

    color = colorCorrection(color);

    gl_FragColor = vec4(color, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
