#ifndef NL_CONFIG_H
#define NL_CONFIG_H

/*
  NEWB SHADER MAIN CONFIG
  This part contains base configuration options for the shader.

  TOGGLES
  - If [toggle] is mentioned, then
    options can be commented to disable (by adding '//')
  - eg: #define PLANTS_WAVE    -> this is ON
        //#define PLANTS_WAVE  -> this is OFF

  COLOR VALUES
  - Color format: vec3(red,green,blue)
  - 1.0 means 100%, 0.0 means 0%
  - eg: vec3(1.0,1.0,0.0) = yellow

  VALUES
  - Values must be decimal
  - eg. 32 is wrong, 32.0 is correct

  TYPES
  - Should be integer. options to choose will be mentioned there
  - eg: #define NL_CLOUD_TYPE 2

  Remember to rebuild the shader after making changes.
*/

/* Color correction */
#define NL_TONEMAP_TYPE 8              // 1:Exponential, 2:Reinhard, 3:Extended Reinhard, 4:ACES 5:John Habel's filmic (used in BSBE) 
#define NL_GAMMA 1.14                  // 0.3 low ~ 2.0 high
//#define NL_EXPOSURE 1.3              // [toggle] 0.5 dark ~ 3.0 bright
#define NL_SATURATION 1.36            // [toggle] 0.0 grayscale ~ 4.0 super saturated
//#define NL_TINT                      // [toggle] enable light/dark tone tinting
#define NL_TINT_LOW  vec3(0.3, 0.5, 1.2) // color tint for dark tone
#define NL_TINT_HIGH vec3(1.3, 0.6, 0.2)  // color tint for light tone

/* Terrain lighting */
#define NL_SUN_INTENSITY 3.8   // 0.5 weak ~ 5.0 bright
#define NL_TORCH_INTENSITY 3.0  // 0.5 weak ~ 3.0 bright
#define NL_NIGHT_BRIGHTNESS 0.0 // 0.0 dark ~ 2.0 bright
#define NL_CAVE_BRIGHTNESS 0.34  // 0.0 dark ~ 2.0 bright
#define NL_SHADOW_INTENSITY 0.85 // 0.0 no shadow ~ 1.0 strong shadow
//#define NL_BLINKING_TORCH     // [toggle] flickering light
//#define NL_CLOUD_SHADOW       // [toggle] cloud shadow (simple clouds only)

/* Sun/moon light color on terrain */
#define NL_MORNING_SUN_COL vec3(0.98, 0.84, 0.73)
#define NL_NOON_SUN_COL    vec3(0.98, 0.90, 0.73)
#define NL_NIGHT_SUN_COL   vec3(0.4039, 0.3804, 0.9804)

/* Ambient light on terrain (light that is added everywhere) */
#define NL_NETHER_AMBIENT vec3(3.0, 2.18, 1.73) //previously vec3(0.5, 0.25, 0.2)
#define NL_END_AMBIENT    vec3(0.23,0.0,0.9)

/* Torch colors */
#define NL_OVERWORLD_TORCH_COL  vec3(1.0,0.52,0.18)
#define NL_UNDERWATER_TORCH_COL vec3(2.30,2.30,2.30)
#define NL_NETHER_TORCH_COL     vec3(1.0,0.52,0.18)
#define NL_END_TORCH_COL        vec3(0.55, 0.24, 0.89)

/* Fog */
#define NL_FOG 0.8                // [toggle] 0.1 subtle ~ 1.0 blend with sky completely
#define NL_MIST_DENSITY 0.0      // 0.0 no mist ~ 1.0 misty
#define NL_RAIN_MIST_OPACITY 0.29 // [toggle] 0.04 very subtle ~ 0.5 thick rain mist blow
//#define NL_CLOUDY_FOG 0.0         // [toggle] 0.0 subtle - 0.8 dense fog clouds

/* Sky colors - zenith=top, horizon=bottom */
#define NL_DAY_ZENITH_COL    vec3(0.133, 0.404, 0.886) //previously vec3(0.1333, 0.5176, 0.9020)
#define NL_DAY_HORIZON_COL   vec3(0.694, 0.847, 0.960) //previously vec3(0.752, 0.902, 0.980)
#define NL_NIGHT_ZENITH_COL  vec3(0.0,0.03,0.28)
#define NL_NIGHT_HORIZON_COL vec3(0.0,0.29,0.68)
#define NL_RAIN_ZENITH_COL   vec3(0.239, 0.231, 0.231)
#define NL_RAIN_HORIZON_COL  vec3(0.451, 0.447, 0.447)
#define NL_END_ZENITH_COL    vec3(0.0,0.0,0.0)
#define NL_END_HORIZON_COL   vec3(0.5,0.4,1.5)
#define NL_DAWN_ZENITH_COL   vec3(0.2078, 0.1451, 0.3333) 
#define NL_DAWN_HORIZON_COL  vec3(1.0, 0.3804, 0.0196) //previously vec3(1.0, 0.2353, 0.0)
#define NL_DAWN_EDGE_COL     vec3(1.0, 0.3804, 0.0196) //previously vec3(1.0, 0.1647, 0.0)

/* Rainbow */
//#define NL_RAINBOW         // [toggle] enable rainbow in sky
#define NL_RAINBOW_CLEAR 0.5 // 0.3 subtle ~ 1.7 bright during clear
#define NL_RAINBOW_RAIN 1.0  // 0.5 subtle ~ 2.0 bright during rain

/* Ore glow intensity */
#define NL_GLOW_TEX 3.0           // 0.4 weak ~ 8.0 bright
//#define NL_GLOW_SHIMMER 0.8       // [toggle] 0.1 subtle ~ 1.0 100% shimmer
#define NL_GLOW_SHIMMER_SPEED 1.0 // 0.5 slow - 2.0 fast
#define NL_GLOW_LEAK 0.4        // [toggle] 0.08 subtle ~ 1.0 100% brightness of NL_GLOW_TEX

/* Waving */
#define NL_PLANTS_WAVE 0.05    // [toggle] 0.02 gentle ~ 0.4 violent
#define NL_LANTERN_WAVE 0.13   // [toggle] 0.05 subtle ~ 0.4 large swing
#define NL_WAVE_SPEED 0.74      // 0.5 slow wave ~ 5.0 very fast wave
//#define NL_EXTRA_PLANTS_WAVE // [toggle] !dont use! wave using texture coords (1.21.0 vanilla 2048x1024)

/* Water */
#define NL_WATER_TRANSPARENCY 0.2 // 0.0 transparent ~ 1.0 normal
#define NL_WATER_BUMP 0.2        // 0.001 plain ~ 0.2 bumpy water
#define NL_WATER_WAVE_SPEED  0.4  // 0.2 calm ~ 2.0 turbulent
#define NL_WATER_TEX_OPACITY 0.0  // 0.0 plain water ~ 1.0 vanilla water texture
#define NL_WATER_WAVE             // [toggle] wave effect
//#define NL_WATER_CLOUD_REFLECTION // [toggle] simple clouds/aurora reflection
//#define NL_WATER_REFL_MASK      // [toggle] fake water reflection mask
#define NL_WATER_TINT vec3(1.3,1.55,1.45)

/* Underwater */
#define NL_UNDERWATER_BRIGHTNESS 0.4         // 0.0 dark ~ 3.0 bright
#define NL_CAUSTIC_INTENSITY 4.9             // 0.5 weak ~ 5.0 bright
#define NL_UNDERWATER_WAVE 0.04               // [toggle] 0.02 subtle ~ 0.6 trippy
#define NL_UNDERWATER_STREAKS 1.8            // [toggle] 0.8 subtle - 2.0 bright streaks from top
#define NL_UNDERWATER_TINT vec3(0.9,1.3,0.9) // fog tint color when underwater

/* Cloud type */
#define NL_CLOUD_TYPE 3 // 0:vanilla, 1:soft, 2:rounded, 3:realistic, 4:Custom shadertoy clouds

/* Vanilla cloud settings - make sure to remove clouds.png when using this */
#define NL_CLOUD0_THICKNESS 2.1      // 0.5 slim ~ 8.0 fat
#define NL_CLOUD0_RAIN_THICKNESS 4.0 // 0.5 slim ~ 8.0 fat
#define NL_CLOUD0_OPACITY 0.9        // 0.0 invisible ~ 1.0 opaque
#define NL_CLOUD0_MULTILAYER         // [toggle] extra cloud layer

/* Soft cloud settings */
#define NL_CLOUD1_SCALE vec2(0.016, 0.022) // 0.003 large ~ 0.2 tiny
#define NL_CLOUD1_DEPTH 1.3                // 0.0 no bump ~ 10.0 large bumps
#define NL_CLOUD1_SPEED 0.04               // 0.0 static ~ 0.4 fast moving
#define NL_CLOUD1_DENSITY 0.54             // 0.1 less clouds ~ 0.8 more clouds
#define NL_CLOUD1_OPACITY 0.9              // 0.0 invisible ~ 1.0 opaque

/* Rounded cloud settings */
#define NL_CLOUD2_THICKNESS 2.1            // 0.5 slim ~ 5.0 fat
#define NL_CLOUD2_RAIN_THICKNESS 2.5       // 0.5 slim ~ 5.0 fat
#define NL_CLOUD2_STEPS 5                  // 3 low quality ~ 16 high quality
#define NL_CLOUD2_SCALE vec2(0.033, 0.033) // 0.003 large ~ 0.3 tiny
#define NL_CLOUD2_SHAPE vec2(0.5, 0.4)     // 0.0 round ~ 1.0 box vec2(horizontal shape, vertical shape)
#define NL_CLOUD2_DENSITY 25.0             // 1.0 blurry ~ 100.0 sharp
#define NL_CLOUD2_VELOCITY 0.8             // 0.0 static ~ 4.0 very fast
//#define NL_CLOUD2_LAYER2                      // [toggle] extra cloud layer
#define NL_CLOUD2_LAYER2_OFFSET 143.0           // 30.0 near ~ 300.0 very high
#define NL_CLOUD2_LAYER2_THICKNESS 2.5          // 0.7 slim ~ 5.0 fat
#define NL_CLOUD2_LAYER2_RAIN_THICKNESS 3.0     // 0.7 slim ~ 5.0 fat
#define NL_CLOUD2_LAYER2_STEPS 3                // 3 low quality ~ 16 high quality
#define NL_CLOUD2_LAYER2_SCALE vec2(0.03, 0.03) // 0.003 large ~ 0.3 tiny
#define NL_CLOUD2_LAYER2_SHAPE vec2(0.5, 0.4)   // 0.0 round ~ 1.0 box vec2(horizontal shape, vertical shape)
#define NL_CLOUD2_LAYER2_DENSITY 25.0           // 1.0 blurry ~ 100.0 sharp
#define NL_CLOUD2_LAYER2_VELOCITY 0.8           // 0.0 static ~ 4.0 very fast

/* Realistic cloud settings */
#define NL_CLOUD3_SCALE vec2(0.05, 0.05) // 0.003 large ~ 1.0 tiny
#define NL_CLOUD3_SPEED 0.01            // 0.0 static ~ 4.0 fast moving
#define NL_CLOUD3_SHADOW 0.59             // 0.1 subtle ~ 1.0 dark
#define NL_CLOUD3_SHADOW_OFFSET 0.79      // 0.05 minimal ~ 1.0 large

/*
// Shadertoy Cloud Parameters (Type 4)
#define CLOUD4_SCALE 1.1      // Cloud pattern scale
#define CLOUD4_SPEED 0.03     // Cloud movement speed
#define CLOUD4_DARK 0.5       // Darkest cloud parts
#define CLOUD4_LIGHT 0.3      // Lightest cloud parts  
#define CLOUD4_COVER 0.2      // Base cloud coverage
#define CLOUD4_ALPHA 8.0      // Cloud density multiplier
#define CLOUD4_SKYTINT 0.5    // Sky color influence on clouds
*/

/* Aurora settings */
// #define NL_AURORA 3.2           // [toggle] 0.4 dim ~ 4.0 very bright
#define NL_AURORA_VELOCITY 0.08 // 0.0 static ~ 0.3 very fast
#define NL_AURORA_SCALE 0.08    // 0.002 large ~ 0.4 tiny
#define NL_AURORA_WIDTH 0.28    // 0.04 thin line ~ 0.4 thick lines
#define NL_AURORA_COL1 vec3(0.1,1.0,0.0)
#define NL_AURORA_COL2 vec3(0.1,0.0,1.0)

/* Shooting star */
#define NL_SHOOTING_STAR 1.0        // [toggle] 0.2 dim ~ 1.0 bright
#define NL_SHOOTING_STAR_PERIOD 1.5 // 0.4 fast ~ 12.0 slow (12 secs)
#define NL_SHOOTING_STAR_DELAY 0.2 // 0.0 no delay ~ 100.0 long time (100 secs)
#define STAR_SCALE             0.25    // 1.0 default ~ >1.0 bigger

/* Galaxy */
#define NL_END_GALAXY_STARS 3.5           // [toggle] 0.1 dim - 4.0 bright
#define NL_GALAXY_VIBRANCE 0.7          // 0.0 white - 1.0 colorful
#define NL_GALAXY_SPEED 0.01            // 0.01 slow motion - 0.2 fast motion
#define NL_GALAXY_DAY_VISIBILITY 1.0    // 0.0 invisible - 1.0 visible

/* Chunk loading slide in animation */
//#define NL_CHUNK_LOAD_ANIM 100.0 // [toggle] -600.0 fall from top ~ 600.0 rise from bottom

/* Sun/Moon */
#define NL_SUNMOON_ANGLE 90.0 // [toggle] 0.0 no tilt ~ 90.0 tilt of 90 degrees
#define NL_SUN_SIZE 0.2     // 0.3 tiny ~ 4.0 massive
#define NL_MOON_SIZE 0.45     //separate moon size scale
/* Fake godrays during sunrise/sunset */
#define NL_GODRAY 0.8 // [toggle] 0.1 subtle ~ 0.8 strong

/* Sky reflection */
#define NL_GROUND_REFL 0.6       // [toggle] 0.2 slightly reflective ~ 1.0 fully reflect sky 
#define NL_GROUND_RAIN_WETNESS 1.0 // 0.0 no wetness ~ 1.0 fully wet blocks when raining
#define NL_GROUND_RAIN_PUDDLES 1.0 // 0.0 no puddles ~ 1.0 puddles
//#define NL_GROUND_AURORA_REFL    // [toggle] aurora reflection on ground

/* Entity (Actor, ItemInHand) */
#define NL_ENTITY_BRIGHTNESS     0.6 // 0.1 dark ~ 1.6 bright
#define NL_ENTITY_EDGE_HIGHLIGHT 0.45 // 0.0 no highlight ~ 1.6 bright highlight

/* Weather particles */
#define NL_WEATHER_SPECK 0.4         // [toggle] 0.0 vanilla texture ~ 1.0 soft speck
#define NL_WEATHER_RAIN_SLANT 3.5    // 1.0 minimal ~ 8.0 violent
#define NL_WEATHER_PARTICLE_SIZE 0.8 // 0.5 tiny ~ 4.0 large

/* Lava effects */
#define NL_LAVA_NOISE            // [toggle] darken lava in certain regions
#define NL_LAVA_NOISE_BUMP 0.32 // [toggle] 0.1 subtle ~ 0.8 massive waves
#define NL_LAVA_NOISE_SPEED 0.02 // 0.0 still ~ 0.1 fast

/* Black Hole */
#define NL_BH_COL_LOW vec3(0.1, 0.1, 1.0)
#define NL_BH_COL_HIGH vec3(2.8, 0.1, 0.6)
#define NL_BH_DIST 1.8
#define NL_BH_SPEED 0.3

/* // Fake PBR reflections by LynxMC
#define DROP_NUMBER  3        // idk, just to define DROP_NUMBER of custom.h */
/*
  NEWB SHADER SUBPACK CONFIG
  This part contains custom configuration options for each subpack.

  If a value is already defined,
  then you must undefine it before modifying:
  eg: #undef OPTION_NAME

  Subpack names and flags are inside `pack_config.toml`.
  Build tool will enable corresponding flags when compiling. 
*/

/* #ifdef NO_WAVE_NO_FOG
  #define NO_WAVE
  #define NO_FOG
#endif

#ifdef NO_FOG
  #undef NL_FOG
#endif

#ifdef NO_WAVE
  #undef NL_PLANTS_WAVE
  #undef NL_LANTERN_WAVE
  #undef NL_UNDERWATER_WAVE
  #undef NL_WATER_WAVE
  #undef NL_RAIN_MIST_OPACITY
#endif 

#ifdef CHUNK_ANIM
  #define NL_CHUNK_LOAD_ANIM 100.0
#endif */

#ifdef ROUNDED_CLOUDS
  #undef NL_CLOUD_TYPE
  #define NL_CLOUD_TYPE 2
#endif

/* #ifdef BOX_CLOUDS
  #undef NL_CLOUD_TYPE
  #define NL_CLOUD_TYPE 0
#endif */

#ifdef NO_AURORA
  #undef NL_AURORA
  #undef NL_AURORA_VELOCITY
  #undef NL_AURORA_SCALE
  #undef NL_AURORA_WIDTH
  #undef NL_AURORA_COL1
  #undef NL_AURORA_COL2
#endif

#endif
