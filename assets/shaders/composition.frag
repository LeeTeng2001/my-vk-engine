#version 450

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorSampler;
layout(set = 0, binding = 1) uniform sampler2D depthSampler;


layout (push_constant) uniform PushConstantData {
    float sobelWidth;
    float sobelHeight;
} pushC;

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
//    outColor = texture(depthSampler, inUV);
//    outColor = vec4(inUV, 0, 1);
//    vec4 x = texture(tex, coord);
//    outColor = vec4(x.r, 0, 0, 1);

    vec4 n[9];
    make_kernel(n, depthSampler, inUV);

    vec4 sobel_edge_h = n[2] + (2.0*n[5]) + n[8] - (n[0] + (2.0*n[3]) + n[6]);
    vec4 sobel_edge_v = n[0] + (2.0*n[1]) + n[2] - (n[6] + (2.0*n[7]) + n[8]);
    vec4 sobel = sqrt((sobel_edge_h * sobel_edge_h) + (sobel_edge_v * sobel_edge_v));
    outColor = vec4(1.0 - sobel.rgb, 1.0 );
}

