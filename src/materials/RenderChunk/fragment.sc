$input v_color0, v_color1, v_fog, v_refl, v_texcoord0, v_lightmapUV, v_extra, v_position, v_wpos

#include <bgfx_shader.sh>
#include <newb/main.sh>
#include <newb/functions/custom.h>
#include <newb/functions/starfield.h>
#include <newb/functions/PBR.h>
#include <newb/functions/puddles.h>


SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_SeasonsTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);
SAMPLER2D_AUTOREG(s_CloudTexture);

uniform vec4 FogAndDistanceControl;
uniform vec4 FogColor;
uniform vec4 DimensionID;
uniform vec4 TimeOfDay;
uniform vec4 ViewPositionAndTime;
uniform vec4 SunDirection;
uniform vec4 WeatherID;
uniform vec4 CameraPosition;

#define CAUSTICS_SPEED 0.5

// Caustics
highp float E_UNDW(vec3 v_position, vec2 v_uv1) {

  highp float SCALE = 0.6;
  highp float TAU2 = 6.283;
  int MAX_ITER = 2;

  highp float time = ViewPositionAndTime.w * CAUSTICS_SPEED + 23.0;
  // uv should be the 0-1 uv of texture...
  vec2 uv = v_position.xz * SCALE;
  vec2 p, i; p = i = mod(uv*TAU2, TAU2)-250.0;
  highp float c = 1.0;
  highp float inten = .005;

  for (int n = 0; n < MAX_ITER; n++) {
  highp float t = time * (1.0 - (3.5 / float(n+1)));
  i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
  c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
  }
  c /= float(MAX_ITER);
  c = 1.17-pow(c, 1.4);
  float colour = clamp(pow(abs(c), 8.0), 0.0, 0.7 ) * ( v_uv1.y + 0.1 ) * ( 1.0 - v_uv1.x ) * 3.1;
  return colour;
}
/*
// Rain ripples
#define rot(x) mat2(cos(x), sin(x), -sin(x), cos(x))

highp float hash(highp vec2 x){
  return fract(sin(dot(x,vec2(11,57)))*4567.0+ViewPositionAndTime.w);
}

highp float ripplenoise(highp vec2 x, highp float k){
  highp float r = hash(floor(x)), d = length(fract(x)-0.5);

  return smoothstep(0.05,0.025,distance(d, r*k))*smoothstep(0.5,0.475,d)*((1.0-r)*k);
}

highp float ripple(highp vec2 x,float time){
  highp float result = 0.0;

    for(int i = 0; i<3; i++){
    x -= 6.0, x = mul(x, mat2(0.94497, 0.32716, -0.32716, 0.94497)),
    result += ripplenoise(x+time*0.5,float(i)/3.0+1.0);
  }

  return result;
}

highp vec3 norm(highp vec2 x, float time) {
  highp float

  m = 0.01,
  r = ripple(x-vec2(m,0),time),
  g = ripple(x-vec2(0,m),time),
  b = ripple(x,time);

  r = r-b,
  g = g-b;
  b = 1.0-(r+g);

  return (vec3(r+0.5,g+0.5,b*m+1.0));
}
*/
vec3 filmicBSBE(vec3 x){
    return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

void main() {
  #if defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY) || defined(INSTANCING)
    gl_FragColor = vec4(1.0,1.0,1.0,1.0);
    return;
  #endif

  vec4 diffuse = texture2D(s_MatTexture, v_texcoord0);
  vec4 texcol  = texture2D(s_MatTexture, v_texcoord0);

  vec3 realPos = v_wpos.xyz + CameraPosition.xyz;
  float camDist = v_wpos.w;
  vec3 viewDir = normalize(v_wpos.xyz);

    vec4 whatTime = timeofday(TimeOfDay.x);
      float night = whatTime.x;
      float day   = whatTime.w;
      float dusk  = whatTime.z;
      float dawn  = whatTime.y;

    float rain          = clamp(WeatherID.x, 0.0, 1.0);

  nl_environment env = nlDetectEnvironment(FogColor.rgb, FogAndDistanceControl.xyz);
  nl_skycolor skycol;
  if (env.underwater) {
     skycol = nlUnderwaterSkyColors(env.rainFactor, FogColor.rgb);
    } else {
     skycol = nlOverworldSkyColors(env.rainFactor,FogColor.rgb);
    }
    
  vec4 color = v_color0;

  vec2 lit =  v_lightmapUV;
  float nolight = 1.0 - lit.y;
  
  //sun angle
  vec3 V = normalize(-viewDir);
  vec3 N = normalize(cross(dFdx(v_position), dFdy(v_position)));
  vec3 sunDir = normalize(SunDirection.xyz);
  vec3 moonDir = normalize(vec3(-0.6, 0.45, -0.7)) * smoothstep(0.0, 0.8, night*night);
  vec3 SunMoonDir = mix(sunDir, moonDir, smoothstep(0.0, 0.8, night*night));
  
  vec3 blockNormal = getNormal(s_MatTexture, v_texcoord0);
  vec3 worldNormal = normalize(mul((blockNormal),getTBN(N)));
  vec3 reflectNormal = reflect(V, worldNormal);

  bool water = v_extra.b > 0.9;

  bool reflective = false;

    const vec3 Ambient = vec3(0.02, 0.04, 0.08);
    bool isCave = nolight > 0.3;
 
 #if !defined(TRANSPARENT) && !defined(ALPHA_TEST)
 bool detecttexture = texcol.a > 0.965 && texcol.a < 0.975; 
 if(detecttexture){
    reflective = true;
        }
 #endif

 #if !defined(TRANSPARENT) && !defined(ALPHA_TEST) 
 #endif

  float shadow = smoothstep(0.875,0.860, pow(v_lightmapUV.y,2.0));
  shadow = mix(shadow, 0.0, pow(v_lightmapUV.x * 1.2, 6.0)); 
  float shadowFactor = 1.0 - 0.25 * shadow;
  shadowFactor = max(shadowFactor, 0.5); 
  diffuse.rgb *= shadowFactor;

  // side block shadows
  float sideshadow = smoothstep(0.64, 0.62, v_color1.g);
  diffuse.rgb *= 1.0-0.3*sideshadow;                     //increase 0.38 = darker shadow

  #if defined(SEASONS) && (defined(OPAQUE) || defined(ALPHA_TEST))
    diffuse.rgb *= mix(vec3(1.0,1.0,1.0), texture2D(s_SeasonsTexture, v_color1.xy).rgb * 2.0, v_color1.z);
  #endif

  vec3 glow = nlGlow(s_MatTexture, v_texcoord0, v_extra.a);

  diffuse.rgb *= diffuse.rgb;
 
  vec3 lightTint = texture2D(s_LightMapTexture, v_lightmapUV).rgb;
  // lightTint = mix(lightTint.bbb, lightTint*lightTint, 0.35 + 0.65*v_lightmapUV.y*v_lightmapUV.y*v_lightmapUV.y);

  #ifndef NL_FULLBRIGHT
    diffuse.rgb *= lightTint;
  #endif

  float isLeaf = 0.0;
  #if defined(SEASONS) && (defined(ALPHA_TEST) || defined(OPAQUE))
    isLeaf = 1.0;
  #endif

  diffuse.rgb *= color.rgb;   // point light & other things
  diffuse.rgb += glow;

  // Caustics
    if (env.underwater){
      float upwards = max(N.y, 0.0);
      vec3 caustics = (E_UNDW(realPos, v_lightmapUV)*vec3(0.75, 0.8, 1.0))*upwards;
      diffuse.rgb *= caustics;
    }

  // water 
  diffuse = applyWaterEffect(s_CloudTexture, realPos, v_wpos.xyz, viewDir, V, diffuse, skycol, env, FogColor.rgb, ViewPositionAndTime.w, night, dusk, dawn, rain, water, v_lightmapUV.y, sunDir, N);

  // /*
  vec3 dawnCol = vec3(1.0, 0.52, 0.278);
  vec3 nightCol = vec3(0.5765, 0.584, 0.98); 
  vec3 dayCol  = vec3_splat(1.0);
  float twilight = saturate(dusk + dawn);
  vec3 specularCol = dawnCol*twilight + dayCol*day + nightCol*smoothstep(0.0, 0.7, night); 
  if (env.underwater) specularCol = mix(vec3(0.1, 0.25, 0.5), specularCol, 0.67);


  vec3 F0 = mix(vec3(0.04, 0.04, 0.04), texcol.rgb, 0.5);
  vec3 specular = brdf(normalize(SunMoonDir), V, 0.2, worldNormal, diffuse.rgb, 0.0, F0, specularCol);
  float fresnel = pow(1.0 - dot(V, worldNormal), 5.0); 
  viewDir = reflect(viewDir, worldNormal);
  
  // firmaments declarations for using in reflections  
  vec3 skyReflection = getSkyRefl(skycol, env, viewDir, FogColor.rgb, ViewPositionAndTime.w);

  vec3 galaxyStars = nlGalaxy(viewDir, FogColor.rgb, env, ViewPositionAndTime.w);

  // specular highlights 
  float specDist = FogAndDistanceControl.z*0.67;
    if(!env.end && !env.nether && v_extra.b<0.9 && !reflective && isLeaf==0.0){
      vec3 specHighlights = brdf_specular(normalize(SunMoonDir), V, worldNormal, 0.65, F0, specularCol);
      diffuse.rgb += specHighlights;
    }

  float downwards = max(-N.y, 0.0);
  float notBottom = 1.0 - downwards;

  vec3 stars;

  if(DimensionID.x == 0.0){
    vec3 fstars = vec3(0.0, 0.0, 0.0);
    vec2 starUV = viewDir.xz / (0.5 + viewDir.y);
    float starValue = star(starUV * NL_FALLING_STARS_SCALE, NL_FALLING_STARS_VELOCITY, NL_FALLING_STARS_DENSITY, ViewPositionAndTime.w);
    float starFactor = smoothstep(0.67, 1.0, night)*(1.0-rain);
    fstars = pow(vec3(starValue, starValue, starValue) * 1.1, vec3(16.0, 6.0, 4.0));
    fstars *= starFactor;
    
    vec3 gstars = NL_GALAXY_STARS * galaxyStars;
    gstars *= gstars;
    stars = (gstars * 2.1 * notBottom) + (fstars*0.7);
  } else {
    stars = renderStarfield(viewDir, ViewPositionAndTime.w) * 0.5; 
  }

  vec3 cloudPos;

  vec3 reflection = skyReflection;
    #if NL_CLOUD_TYPE == 3
      cloudPos.xz = 3.0 * viewDir.xz / viewDir.y;
      vec4 clouds = renderClouds(cloudPos.xz, 0.1 * ViewPositionAndTime.w, rain, skycol.horizonEdge, skycol.zenith, NL_CLOUD3_SCALE, NL_CLOUD3_SPEED, NL_CLOUD3_SHADOW);
      reflection = mix(reflection, clouds.rgb, clouds.a * smoothstep(0.05,1.0,viewDir.y));
    #else
      cloudPos.xz = 36.0 * viewDir.xz/max(viewDir.y, 0.05);
      cloudPos.y = 1.0*viewDir.y;
      vec4 clouds = renderCloudsRounded(s_CloudTexture, viewDir, cloudPos, rain, ViewPositionAndTime.w, skycol.horizonEdge, skycol.zenith, NL_CLOUD_PARAMS(_)); 
    #endif 
  
  if(DimensionID.x == 0.0) reflection = mix(reflection, clouds.rgb, clouds.a * smoothstep(0.05,1.0,viewDir.y));

  if(reflective && !water && !env.nether){
    reflection  += stars;
    diffuse.rgb *= 1.0 - F0;
    float notDown = diffuse.a * fresnel * notBottom;
    diffuse.rgb = mix(diffuse.rgb, reflection, notDown);
    diffuse.rgb += specular * notBottom; 
  } 
  // */ 

  diffuse.rgb = mix(diffuse.rgb, v_fog.rgb, v_fog.a);

  diffuse.rgb = colorCorrection(diffuse.rgb);

  #ifdef ALPHA_TEST
    if (diffuse.a < 0.5) {
      discard;
    }
  #endif

  gl_FragColor = diffuse;
}
