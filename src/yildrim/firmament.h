#ifndef FIRMAMENT_H
#define FIRMAMENT_H

// Falling stars by i11212
highp float randS(highp vec2 x){
    return fract(sin(dot(x, vec2(11,57))) * 4e3);
}

highp float star(highp vec2 x, highp float speed, highp float density, highp float time){
    x = mul(x, mtxFromCols(vec2(cos(0.0), sin(0.0)), vec2(sin(0.0), -cos(0.5))));
    x.y += time * speed;  // Use speed parameter
    
    highp float shape = (1.0 - length(fract(x - vec2(0, 0.5)) - 0.5));
    x *= vec2(1, 0.1);
    
    highp vec2 fr = fract(x);
    highp float random = step(randS(floor(x)), density);  // Use density parameter
    highp float tall = (1.0 - (abs(fr.x - 0.5) + fr.y * 0.5)) * random;
    
    return clamp(clamp((shape - random) * step(randS(floor(x + vec2(0, 0.05))), density), 0.0, 1.0) + tall, 0.0, 1.0);
}

// Lynx Aurora (3D) 
mat2 mm2(in float a){float c = cos(a), s = sin(a);return mat2(c,s,-s,c);}
mat2 m2 = mat2(0.1553, 0.1955, -0.1955, 0.1553);

float tri(float x) {
    x = fract(x);               // 0..1
    x = x < 0.5 ? x : 1.0 - x;  // triangle wave
    return clamp(x, 0.01, 0.49);
}

vec2 tri2(vec2 p) {
    float tx = tri(p.x);
    float ty = tri(p.y);
    return vec2(tx + ty, tri(p.y + tx));
}
float triNoise2d(in vec2 p, float spd,float time)
{
    float z=1.9;
    float z2=1.1;
    float rz = 0.;
      p = mul(mm2(p.x * 0.06), p);
    vec2 bp = p;
    for (int i=0; i<3; i++ )
    {
        vec2 dg = tri2(bp*1.85)*.79;
        dg = mul(mm2(time*spd), dg);
        p -= dg/z2;

        bp *= 1.3;
        z2 *= .45;
        z *= .42;
        p *= 1.21 + (rz-1.0)*.02;

        rz += tri(p.x+tri(p.y))*z;
        p = mul(-m2, p);
    }
    return clamp(1./pow(rz*29., 1.3),0.,.55);
}
vec4 rdAurora(vec3 ro, vec3 rd, nl_environment env, float time, vec3 FOG_COLOR, float rain) {
    vec4 col = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 avgCol = vec4(0.0, 0.0, 0.0, 0.0);

    // Estimate distance to aurora band (XZ plane)
    float distXZ = length(ro.xz); // Or use offset if aurora is not at origin

    // LOD based on distance
    int steps = 1; // default
    if (distXZ < 20.0) steps = 12;       // Near (LOD 3)
    else if (distXZ < 60.0) steps = 8;   // Mid (LOD 2)
    else steps = 1;                      // Far (LOD 1)

    float invSteps = 1.0 / float(steps);
    float waveTime = time * 0.01;

    for (int i = 0; i < 16; i++) {
        if (i >= steps) break; // Only process up to chosen LOD steps

        float stepRatio = float(i) * invSteps;
        float height = 0.95 + stepRatio * 0.18;
        float pt = (height - ro.y) / (rd.y * 0.2 + 0.4);
        vec3 bpos = ro + pt * rd;

        float wave = bpos.y * 2.0;
        vec2 p = bpos.zx + vec2(
            sin(bpos.y * 2.0 + time * 0.5),
            cos(bpos.y * 2.0 + time * 0.3)
        ) * 0.05 + time * 0.01;

        float rzt = triNoise2d(p, 0.2, time);

        float colorFactor = sin(float(i) * 0.15 + time * 0.2) * 0.5 + 0.5;
        vec3 auroraColor = vec3(0.0, 0.0, 0.0);

        if(!env.end && !env.nether){
          auroraColor = mix(vec3(0.102, 0.565, 1.000) /* vec3(0.6, 0.2, 1.0) */ ,vec3(0.102, 0.565, 1.000), colorFactor);
        } else {
          auroraColor = mix(vec3(0.447, 0.035, 0.718), vec3(0.447, 0.035, 0.718), colorFactor); 
        }
        vec4 col2 = vec4(auroraColor * rzt, rzt);
        avgCol = mix(avgCol, col2, 0.4);
        col += avgCol * exp2(-stepRatio * 2.4 - 2.0);
    }

    col *= clamp(rd.y * 10.0 + 0.2, 0.0, 1.0);

    float mask = (1.0 - 0.5 * rain) * max(1.0 - 2.0 * max(FOG_COLOR.b, FOG_COLOR.g), 0.0);
    return saturate(col * 1.8 * mask);
}

#endif