#version 450
#extension GL_ARB_gpu_shader_int64 : enable

layout (location = 0) out vec3 fragColor;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

layout (push_constant) uniform PushConstantData {
    vec3 vertexOffset;
    uint64_t time;
} pushC;

void main() {
//    float sinVal = sin(pushC.time * 0.001);
//    if (sinVal < 0) sinVal *= -1;
//    gl_Position = vec4(inPosition + pushC.vertexOffset * sinVal, 1.0);
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
