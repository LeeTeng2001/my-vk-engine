#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;

layout (push_constant) uniform PushConstantData {
    vec3 vertexOffset;
    float time;
} pushC;

void main() {
    float sinVal = sin(pushC.time * 0.001);
    if (sinVal < 0) sinVal *= -1;
    outColor = vec4(fragColor * sinVal, 1.0);
}

