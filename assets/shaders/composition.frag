#version 450
#define SOBEL_THRESHOLD 0.4

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorSampler;
layout(set = 0, binding = 1) uniform sampler2D depthSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;


layout (push_constant) uniform PushConstantData {
    float sobelWidth;
    float sobelHeight;
} pushC;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// https://gist.github.com/Hebali/6ebfc66106459aacee6a9fac029d0115
void make_kernel(inout vec4 n[9], sampler2D tex, vec2 coord) {
    vec2 tex_offset = vec2(pushC.sobelWidth, pushC.sobelHeight) / textureSize(tex, 0);

    n[0] = texture(tex, coord + vec2( -tex_offset.r, -tex_offset.g));
    n[1] = texture(tex, coord + vec2(0.0, -tex_offset.g));
    n[2] = texture(tex, coord + vec2(  tex_offset.r, -tex_offset.g));
    n[3] = texture(tex, coord + vec2( -tex_offset.r, 0.0));
    n[4] = texture(tex, coord);
    n[5] = texture(tex, coord + vec2(  tex_offset.r, 0.0));
    n[6] = texture(tex, coord + vec2( -tex_offset.r, tex_offset.g));
    n[7] = texture(tex, coord + vec2(0.0, tex_offset.g));
    n[8] = texture(tex, coord + vec2(  tex_offset.r, tex_offset.g));
}

void main() {
    // add some noise to the central position
    vec2 uv_offset = rand(inUV) / 1.5 * vec2(pushC.sobelWidth, pushC.sobelHeight) / textureSize(normalSampler, 0);

    vec4 n[9];
    make_kernel(n, normalSampler, inUV + uv_offset);

    vec4 sobel_edge_h = n[2] + (2.0*n[5]) + n[8] - (n[0] + (2.0*n[3]) + n[6]);
    vec4 sobel_edge_v = n[0] + (2.0*n[1]) + n[2] - (n[6] + (2.0*n[7]) + n[8]);
    vec4 sobel = sqrt((sobel_edge_h * sobel_edge_h) + (sobel_edge_v * sobel_edge_v));
    // The prominent edge will be bright R
    if (sobel.r > SOBEL_THRESHOLD || sobel.g > SOBEL_THRESHOLD || sobel.b > SOBEL_THRESHOLD) {
        outColor = vec4(1, 1, 1, 1.0);
    } else {
        outColor = texture(colorSampler, inUV);
    }
}

