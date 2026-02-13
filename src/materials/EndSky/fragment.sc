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

// #define LYNX_SMOKE

// Smoke/ End Clouds
#define Author LYNXIUMMC
//noise
highp float rand3(highp vec2 x){
    return fract(sin(dot(x, vec2(14.56, 56.2))) * 20000.);
}

highp float noise(highp vec2 x){
    highp vec2 ipos = floor(x);
    highp vec2 fpos = fract(x);
    fpos = smoothstep(0., 1., fpos);
    float a = rand3(ipos), b = rand3(ipos + vec2(1, 0)),
        c = rand3(ipos + vec2(0, 1)), d = rand3(ipos + 1.);
 return mix(a, b, fpos.x) + (c - a) * fpos.y * (1.0 - fpos.x) + (d - b) * fpos.x * fpos.y;
}

highp float Clouds(vec2 uv){
    float a = 1.;
    float b = .1;
    const int pvp = 3;
    for(int lop = 0; lop < pvp; lop++){
        b+= noise(uv)/a;
        a *= 2.5;
        uv *= 3.;
        }
        return 1.-pow(0.1,max(.8-b,.0));
}

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
    //color += 0.2 * diffuse.rgb;

    // Lynx clouds/ Smoke
    #ifdef LYNX_SMOKE
      vec2 uv0 = viewDir.xz/viewDir.y;
      float lynx = Clouds(uv0 + v_posTime.w * 0.1);
      color = mix(color, vec3(1.1, 0.4, 1.7), lynx);
    #endif
    
    // Add the starfield effect
    #ifdef STARFIELD 
      color += renderStarfield(viewDir, v_posTime.w) * 0.5;
    #endif

    #ifdef END_LYNX_AURORA
      vec4 aurora = rdAurora(v_posTime.xyz*0.001, viewDir, env, v_posTime.w, vec3(0.0,0.0,0.0), 0.0);
      color = mix(color, aurora.rgb*0.5, aurora.a);
    #endif 

    /* 
    vec4 bh = renderBlackhole(normalize(v_posTime.xyz), v_posTime.w);
    color = mix(color, bh.rgb, bh.a);
    */

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
