#version 450
#extension GL_ARB_gpu_shader_int64 : enable

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;

layout (push_constant) uniform PushConstantData {
    mat4 viewTransform;
    uint64_t time;
} pushC;

void main() {
//    float sinVal = sin(pushC.time * 0.001);
//    if (sinVal < 0) sinVal *= -1;
//    outColor = vec4(fragColor * sinVal, 1.0);
    outColor = vec4(fragColor, 1.0);

    // Assuming light source is the same as position of camera.
//    outColor = vec4(fragColor, 1.0);
}

