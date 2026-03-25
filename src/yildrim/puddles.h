#ifndef PUDDLES_H
#define PUDDLES_H

// 2D worley noise for random pattern

//https://github.com/patriciogonzalezvivo/lygia/blob/main/generative/worley.hlsl

#define WORLEY_JITTER 1.5
#define RANDOM_SCALE vec4(143.897, 141.423, 97.3, 109.9)


float distEuclidean(vec2 a, vec2 b) { return distance(a, b); }

vec2 random2(vec2 p) {
    vec3 p3 = fract(p.xyx * RANDOM_SCALE.xyz);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx + p3.yz) * p3.zy);
}

vec2 worley2(vec2 p){
    vec2 n = floor( p );
    vec2 f = fract( p );

    float distF1 = 1.0;
    float distF2 = 1.0;
    vec2 off1 = vec2(0.0, 0.0); 
    vec2 pos1 = vec2(0.0, 0.0);
    vec2 off2 = vec2(0.0, 0.0);
    vec2 pos2 = vec2(0.0, 0.0);
    for( int j= -1; j <= 1; j++ ) {
        for( int i=-1; i <= 1; i++ ) {	
            vec2  g = vec2(i,j);
            vec2  o = random2( n + g ) * WORLEY_JITTER;
            vec2  p = g + o;
            float d = distEuclidean(p, f);
            if (d < distF1) {
                distF2 = distF1;
                distF1 = d;
                off2 = off1;
                off1 = g;
                pos2 = pos1;
                pos1 = p;
            }
            else if (d < distF2) {
                distF2 = d;
                off2 = g;
                pos2 = p;
            }
        }
    }

    return vec2(distF1, distF2);
}


#endif 
