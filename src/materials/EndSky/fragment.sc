#ifndef INSTANCING
$input v_texcoord0, v_posTime, v_fogColor
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>

  SAMPLER2D_AUTOREG(s_SkyTexture);
#endif
#include <newb/config.h>
#include <yildrim/starfield.h>
#include <yildrim/cloud.h>

uniform vec4 ViewPositionAndTime;

#define LYNX_SMOKE

float randC( vec2 p){
 return fract(cos(p.x +p.y * 13.0) * 335.552);
}

float snoise(  vec2 p) {
   vec2 i = floor(p);
   vec2 f = fract(p);
   vec2 u = pow(f, vec2(2.0, 2.0))*(2. - 1.0*f);

   return mix(mix(randC(i+vec2(0.,0.)), randC(i+vec2(1.,0.)), u.x), mix(randC(i+vec2(0.,1.)), randC(i+vec2(1.,1.)), u.x), u.y);
}

float fBM(vec2 p,float x){
 float t = ViewPositionAndTime.w;
 float a = 0.;
 float b = 1.;
 p += t*vec2(0.09,0.02);
for(int i=0; i<4; i++){
 a += snoise(p)*x/b;
 b *= 1.7;
 p *= 2.6;
 p += t*sin(float(i));
}

 return max(1.-a,0.);
}

vec3 cloudEND(vec3 sky, vec3 p) {
    // 1. Robust Projection
    // We use a small epsilon and a tighter scale to bring the clouds "closer"
    float py = abs(p.y) + 0.001; 
    vec2 uv = (p.xz / py) * 0.4; 
    
    // Slower, more 'ethereal' movement
    float t = ViewPositionAndTime.w * 0.015;
    uv += vec2(t, t * 0.3);

    // 2. Horizon and Zenith Masks
    // This ensures clouds stay in a 'belt' around the mid-sky
    float horizonMask = smoothstep(0.02, 0.2, p.y);
    float zenithMask = smoothstep(1.0, 0.5, p.y);
    float finalMask = horizonMask * zenithMask;

    // 3. Boosting the Noise (The 'Visibility' Fix)
    // We sample the noise and then "overdrive" it to ensure it's visible
    float noiseVal = fBM(uv, 1.2); 
    
    // We use a very low edge (0.1) to make sure even weak noise shows up
    float alpha = smoothstep(0.1, 0.6, noiseVal) * finalMask;
    
    // 4. End Dimension Palette (British English: Colour)
    // Using high-saturation purples to contrast against the dark sky
    vec3 deepVoid  = vec3(0.2, 0.0, 0.3);   // Dark Purple
    vec3 endGlow   = vec3(0.7, 0.2, 0.9);   // Bright Magenta
    vec3 starWhite = vec3(0.9, 0.7, 1.0);   // 3D Rim Highlight

    // 5. Adding 3D "Pop"
    // Calculate a 'slope' for lighting by offsetting the UV slightly
    float slope = noiseVal - fBM(uv + vec2_splat(0.01), 1.0);
    vec3 cloudColor = mix(deepVoid, endGlow, noiseVal);
    cloudColor += smoothstep(0.0, 0.1, slope) * starWhite * 0.5;

    // 6. Traditional Final Composition
    // Increase the alpha intensity (1.5x multiplier) for better visibility
    vec3 finalRGB = mix(sky, cloudColor, clamp(alpha * 1.5, 0.0, 1.0));

    return finalRGB;
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
      color = cloudEND(color, viewDir);
    #endif
    
    // Add the starfield effect
    #ifdef STARFIELD 
      color += renderStarfield(viewDir + v_posTime.w*0.0001, v_posTime.w*0.001) * 0.5;
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
