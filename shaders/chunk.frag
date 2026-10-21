#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUv;
layout(location = 3) flat in float vTexLayer;
layout(location = 4) in float vFogDepth;

layout(push_constant) uniform Push {
    mat4 mvp;
    vec4 lightDir;
    vec4 fogColorFar;
    vec4 cameraCurvature;
    float fogNear;
    float fogFar;
    uint enableCurvature;
} pc;

layout(set = 0, binding = 0) uniform sampler2DArray uTexArray;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(vNormal);
    vec3 light = normalize(pc.lightDir.xyz);
    float ndotl = max(dot(n, light), 0.15);
    vec4 tex = texture(uTexArray, vec3(vUv, vTexLayer));
    vec3 lit = tex.rgb * ndotl;

    float fogFar = pc.fogColorFar.w;
    float fog = smoothstep(pc.fogNear, fogFar, vFogDepth);
    vec3 fogColor = pc.fogColorFar.rgb;
    vec3 color = mix(lit, fogColor, fog);

    outColor = vec4(color, tex.a);
}
