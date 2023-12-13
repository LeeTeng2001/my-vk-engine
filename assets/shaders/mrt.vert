#version 450
#extension GL_ARB_gpu_shader_int64 : enable
//#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;

layout (push_constant) uniform PushConstantData {
    mat4 viewTransform;
    vec3 sunPos;
    uint64_t time;
} pushC;

void main() {
    gl_Position = pushC.viewTransform * vec4(inPosition, 1.0);
    // Very simple shader, assuming the position of the sun is coming from camera
    fragColor = vec3(1, 1, 1) * dot(normalize(pushC.sunPos), inNormal);
}
