#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

/* Sun & Moon */
vec3 sunS(vec3 sunDir, vec3 viewDir, float dusk, float dawn, float night) {
  float sunDot = max(0.0, 1.0 - dot(sunDir, viewDir));
  float m = 0.008 / (0.0001 + sunDot);
  m = pow(m, 1.2) * 0.1;
  vec3 sunCol = vec3(1.2, 0.95, 0.72);
  vec3 dawnCol  = vec3(1.0, 0.35, 0.05); 
  sunCol = mix(sunCol, dawnCol, saturate(dawn+dusk));

  return sunCol * m;
}

vec3 getMoon(vec3 moonDir, vec3 viewDir, float night){
    float moonDot = saturate(dot(moonDir, viewDir));
    float core =   pow(smoothstep(0.998, 1.0, moonDot), 0.26);
    float corona = pow(moonDot, 32.0) * max(0.8 - 0.7 * night, 0.0);
    float outerGlow = pow(moonDot, 2.0) * max(0.3 - 0.2 * night, 0.0);
    float moon = core + corona + outerGlow;
    vec3 moonCol = vec3(0.8, 0.9, 1.0);

    return moonCol * (core * 20.0 + corona * 2.0 + outerGlow * 1.5);
}

/* Atmospheric Scattering by Lynx */

#define SKY_BRIGHTNESS 15.0

vec2 raysi(vec3 r0, vec3 rd, float sr) {
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    // No intersection when result.x > result.y
    float a = dot(rd, rd);
    float b = 1. * dot(rd, r0);
    float c = dot(r0, r0) - (sr * sr);
    float d = (b*b) - 4.0*a*c;
    if (d < 0.0) return vec2(1e2,-1e5);
    return vec2(
        (-b - sqrt(d))/(2.0*a),
        (-b + sqrt(d))/(2.0*a)
    );
}


vec3 atmosphere(vec3 r, vec3 r0,
vec3 pSun, float iSun,
float rPlanet, float rAtmos,
vec3 kRlh, float kMie, float shRlh,
float shMie, float g,out vec3 sunColor) {

    vec2 psi = raysi(r0, r, rAtmos);
    if (psi.x > psi.y) return vec3(0.0, 0.0, 0.0);
    psi.y = min(psi.y, raysi(r0, r, rPlanet).x);
    float iStepSize = (psi.y - psi.x ) / 8.0;
    float iTime = 0.0;
    vec3 totalRlh = vec3_splat(0.0);
    vec3 totalMie = vec3_splat(0.0);
    float iOdRlh = 0.0;
    float iOdMie = 0.0;
    float mu = dot(r, pSun);
    float mumu = mu * mu;
    float gg = g * g;
    float pRlh = clamp(3.0 / (12.0 * PI) * (1.0 + mumu), 0.0, 1.0);
    float pMie = clamp(3.0 / (22.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg)), 0.0, 1.0);
    vec3 iPos = r0 + r * (iTime + iStepSize * 0.5);
    float iHeight = length(iPos) - rPlanet;
    float odStepRlh = exp2(-iHeight / shRlh) * iStepSize;
    float odStepMie = pRlh*exp2(-iHeight / shMie) * iStepSize;
    iOdRlh += odStepRlh;
    iOdMie += odStepMie;
    float jStepSize = raysi(iPos, pSun, rAtmos).y / 5.0;
    float jTime = 0.0;
    float jOdRlh = 0.0;
    float jOdMie = 0.0;
    vec3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);
    float jHeight = length(jPos) - rPlanet;
    jOdRlh += exp(-jHeight / shRlh) * jStepSize;
    jOdMie += exp(-jHeight / shMie) * jStepSize;
    jTime += jStepSize;
    vec3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));
    totalRlh += odStepRlh * attn;
    totalMie += odStepMie * attn;
    iTime += iStepSize;
    sunColor = attn;

     return iSun * (pRlh * kRlh * totalRlh + pMie * totalRlh*5e-6);
}

vec3 getSkyColor(vec3 pos, vec3 sunDir,out vec3 sunCol){
	vec3 ro = vec3(0,6372.0e3,0);
    float psCat = SKY_BRIGHTNESS; // brighness
    float rPl = 6372e3;
    float rAt = 6471e3;
    vec3 rlh = vec3(5.5e-6, 13e-6, 32.4e-6);
    vec3 rlp = vec3_splat(13e-6);
    float kMi = 37e-6;
    float rshl = 10e3;
    float rlm = 1.25e3;
    float attn = 0.58;
    
	vec3 color = atmosphere(pos, ro, sunDir, psCat, rPl, rAt, rlh, kMi, rshl, rlm, attn, sunCol);

    vec3 SKY_NIGHT_HORIZON = vec3(0.05, 0.08, 0.15);
    vec3 SKY_NIGHT_ZENITH  = vec3(0.01, 0.03, 0.08);

    float sunFactor = smoothstep(-0.1, 0.1, sunDir.y);
	color += mix(SKY_NIGHT_HORIZON,SKY_NIGHT_ZENITH, smoothstep(1.0,-0.5,pos.y));

return color;
}

#endif