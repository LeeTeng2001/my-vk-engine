#version 450

#define SOBEL_THRESHOLD 0.4
#define LIGHT_COUNT 6
#define SPEC_SHININESS 64  // higher is more subtle
#define SPEC_STRENGTH 0.5  // higher contribute more

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D depthSampler;
layout(set = 0, binding = 1) uniform sampler2D colorSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D positionSampler;

struct PointLight {
    vec4 position;
    vec4 colorAndRadius; // fourth component is light radius
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
};

//struct MaterialInfo {
//
//};

layout(set = 1, binding = 0) uniform UBO {
    PointLight lights[LIGHT_COUNT];
    vec4 camPos;
} ubo;

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

void drawSobet() {
    // DRAW color texture with border with sobel filter
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

void drawBlinnPhong() {
    // Simple shader with blinn-phong shader
    // retrieve data from G-buffer
    vec3 fragPos = texture(positionSampler, inUV).rgb;
    vec3 normal = texture(normalSampler, inUV).rgb;
    vec3 albedo = texture(colorSampler, inUV).rgb;

    // then calculate lighting as usual
    vec3 lighting = albedo * 0.02; // hard-coded ambient component

    vec3 viewDir = normalize(ubo.camPos.xyz - fragPos);
    for (int i = 0; i < LIGHT_COUNT; ++i) {
        // early stop condition
        if (ubo.lights[i].colorAndRadius.rgb == vec3(0, 0, 0)) {
            break;
        }
        // TODO: check for light distance

        // diffuse
        vec3 lightDir = normalize(ubo.lights[i].position.xyz - fragPos);
        vec3 diffuse = max(dot(normal, lightDir), 0.0) * albedo * ubo.lights[i].colorAndRadius.xyz;
        lighting += diffuse;
        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = clamp(pow(max(dot(normal, halfwayDir), 0.0), SPEC_SHININESS), 0, 1);
        vec3 specular = SPEC_STRENGTH * spec * ubo.lights[i].colorAndRadius.xyz;
        lighting += specular;
    }

    // https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
    lighting = clamp(lighting, 0, 1);
    float luminance = (0.299*lighting.r + 0.587*lighting.g + 0.114*lighting.b);

    outColor = vec4(lighting, 1.0);
    //    outColor = vec4(luminance, luminance, luminance, 1.0);
    //    if (luminance < 0.4) {
    //        outColor = vec4(0.2,0.2,0.2, 1.0);
    //    } else if (luminance < 0.8) {
    //        outColor = vec4(0.6,0.2,0.2, 1.0);
    //    } else {
    //        outColor = vec4(1,0.2,0.2, 1.0);
    //    }
}

void main() {
    drawSobet();

    // NOTE: gamma correction is already being handled by SRGB frambuffer,
    // https://learnopengl.com/Advanced-Lighting/Gamma-Correction
}

