#version 450

layout (location = 0) out vec3 fragColor;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

layout (push_constant) uniform PushConstantData {
    vec3 vertexOffset;
    float time;
} pushC;

void main() {
    float sinVal = sin(pushC.time * 0.001);
    if (sinVal < 0) sinVal *= -1;
    gl_Position = vec4(inPosition + pushC.vertexOffset * sinVal, 1.0);
    fragColor = inColor;
}
