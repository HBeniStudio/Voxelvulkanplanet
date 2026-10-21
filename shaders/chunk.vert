#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;
layout(location = 3) in float inTexLayer;

layout(push_constant) uniform Push {
    mat4 mvp;
    vec4 lightDir;
    vec4 fogColorFar;
    vec4 cameraCurvature;
    float fogNear;
    float fogFar;
    uint enableCurvature;
} pc;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUv;
layout(location = 3) flat out float vTexLayer;
layout(location = 4) out float vFogDepth;

void main() {
    vec3 worldPos = inPos;
    if (pc.enableCurvature != 0u) {
        vec3 cam = pc.cameraCurvature.xyz;
        float strength = pc.cameraCurvature.w;
        float dist = length(worldPos - cam);
        worldPos.y -= dist * dist * strength;
    }

    vWorldPos = worldPos;
    vNormal = inNormal;
    vUv = inUv;
    vTexLayer = inTexLayer;
    vFogDepth = length(worldPos - pc.cameraCurvature.xyz);

    gl_Position = pc.mvp * vec4(worldPos, 1.0);
}
