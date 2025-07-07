#ifndef INSTANCING
  $input v_fogColor, v_worldPos, v_underwaterRainTime, v_posTime
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>
  uniform vec4 FogAndDistanceControl;
#endif

uniform vec4 FogColor;
uniform vec4 ViewPositionAndTime;

// Shooting stars by i11212
highp float hash_stars(highp vec2 x) {
    return fract(sin(dot(x, vec2(11, 57))) * 4e3);
}

highp float star(highp vec2 x) {
    float time = ViewPositionAndTime.w;
    x = mul(mat2(cos(0.5), -sin(0.5), sin(0.5), cos(0.5)), x);
    x.y += time * 16.0;
    highp float shape = (1.0 - length(fract(x - vec2(0, 0.5)) - 0.5));
    x *= vec2(1, 0.1);
    highp vec2 fr = fract(x);
    highp float random = step(hash_stars(floor(x)), 0.01),
              tall = (1.0 - (abs(fr.x - 0.5) + fr.y)) * random;
    return clamp(clamp((shape - random) * step(hash_stars(floor(x + vec2(0, 0.05))), 0.01), 0.0, 1.0) + tall, 0.0, 1.0);
}

// Cloud parameters (adjusted for better visibility)
const float cloudscale = 0.8; // Reduced from 1.1 for larger clouds
const float speed = 0.01; // Slower movement
const float cloudcover = 0.5; // Increased coverage
const float cloudalpha = 6.0; // Increased opacity
const mat2 m = mat2(1.6, 1.2, -1.2, 1.6);

vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise(in vec2 p) {
    const float K1 = 0.366025404;
    const float K2 = 0.211324865;
    vec2 i = floor(p + (p.x + p.y) * K1);    
    vec2 a = p - i + (i.x + i.y) * K2;
    vec2 o = (a.x > a.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;
    vec3 h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
    vec3 n = h * h * h * h * vec3(dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
    return dot(n, vec3(70.0, 70.0, 70.0));    
}

float fbm(vec2 n) {
    float total = 0.0, amplitude = 0.1;
    for (int j = 0; j < 7; j++) {
        total += noise(n) * amplitude;
        n = mul(m, n);
        amplitude *= 0.4;
    }
    return total;
}

vec4 renderClouds(vec3 pos, float time, float rainFactor) {
    vec3 p = pos * cloudscale;
    vec2 uv = p.xz / (0.5 + p.y);
    
    float timeFactor = time * speed;
    float q = fbm(uv * 0.5);
    
    //ridged noise shape
    float r = 0.0;
    uv *= cloudscale;
    uv -= q - timeFactor;
    float weight = 0.8;
    for (int l = 0; l < 8; l++) {
        r += abs(weight * noise(uv));
        uv = mul(m, uv) + timeFactor;
        weight *= 0.7;
    }
    
    //noise shape
    float f = 0.0;
    uv = p.xz;
    uv *= cloudscale;
    uv -= q - timeFactor;
    weight = 0.7;
    for (int k = 0; k < 8; k++) {
        f += weight * noise(uv);
        uv = mul(m, uv) + timeFactor;
        weight *= 0.6;
    }
    
    f *= r + f;
    
    //noise colour
    float c = 0.0;
    timeFactor = time * speed * 2.0;
    uv = p.xz;
    uv *= cloudscale * 2.0;
    uv -= q - timeFactor;
    weight = 0.4;
    for (int o = 0; o < 7; o++) {
        c += weight * noise(uv);
        uv = mul(m, uv) + timeFactor;
        weight *= 0.6;
    }
    
    // Combine results
    f = cloudcover + cloudalpha * f * r;
    vec3 cloudColor = vec3(1.0, 1.0, 1.0) * (0.7 + 0.3 * c); // Bright white clouds
    
    // Simple fade at horizon
    float horizonFade = smoothstep(0.0, 0.3, p.y);
    return vec4(cloudColor, clamp(f, 0.0, 1.0) * horizonFade * (1.0 - rainFactor * 0.7));
}

void main() {
  #ifndef INSTANCING
    vec3 viewDir = normalize(v_worldPos);

    nl_environment env;
    env.end = false;
    env.nether = false;
    env.underwater = v_underwaterRainTime.x > 0.5;
    env.rainFactor = v_underwaterRainTime.y;

    nl_skycolor skycol;
    if (env.underwater) {
      skycol = nlUnderwaterSkyColors(env.rainFactor, v_fogColor.rgb);
    } else {
      skycol = nlOverworldSkyColors(env.rainFactor, v_fogColor.rgb);
    }

    vec3 skyColor = nlRenderSky(skycol, env, -viewDir, v_fogColor, v_underwaterRainTime.z);
    
    // Simple cloud rendering
    if (!env.underwater && !env.nether) {
        vec4 clouds = renderClouds(v_worldPos, ViewPositionAndTime.w, env.rainFactor);
        skyColor = mix(skyColor, clouds.rgb, clouds.a);
    }
    
    // Rest of your effects...
    #ifdef NL_SHOOTING_STAR
      skyColor += NL_SHOOTING_STAR*nlRenderShootingStar(viewDir, v_fogColor, v_underwaterRainTime.z);
    #endif

    float night = pow(max(min(1.0 - FogColor.r * 1.5, 1.0), 0.0), 1.2);

    if(night>0.5 && !env.underwater && !env.nether){
      vec2 starUV = viewDir.xz / (0.5 + viewDir.y);
      float starValue = star(starUV * 24.0);
      vec3 starColor = pow(vec3(starValue, starValue, starValue) * 1.1, vec3(16, 6, 4));
      skyColor += starColor;
    }

    skyColor = colorCorrection(skyColor);

    gl_FragColor = vec4(skyColor, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
