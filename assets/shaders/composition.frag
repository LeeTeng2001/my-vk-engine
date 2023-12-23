#version 450
#extension GL_GOOGLE_include_directive : enable

#include "util.glsl"

#define SOBEL_THRESHOLD 0.4
#define SOBEL_OUTLINE_COLOR vec4(0, 0, 0, 1.0)
#define SOBEL_WIGGLE_FACTOR 3  // higher is more wiggly
#define LIGHT_COUNT 6
#define SPEC_SHININESS 32  // higher is more subtle
#define SPEC_STRENGTH 0.4  // higher contribute more

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
    vec4 direction;
    vec4 color;
};

layout(set = 1, binding = 0) uniform UBO {
    DirectionalLight globalDirLight;
    PointLight lights[LIGHT_COUNT];
    vec4 camPos;
} ubo;

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

void drawSobet() {
    // DRAW color texture with border with sobel filter
    // add some noise to the central position

    // draw the smooth curve, this does some cheap smooth linear interpolation
//    vec2 uv_offset = rand(inUV) / SOBEL_WIGGLE_FACTOR * vec2(pushC.sobelWidth, pushC.sobelHeight) / textureSize(normalSampler, 0);
    float noiseVal = SOBEL_WIGGLE_FACTOR * smoothwiggle(inUV.x + (inUV.y * 0.7), 20, 128);
    vec2 uv_offset = noiseVal * vec2(pushC.sobelWidth, pushC.sobelHeight) / textureSize(normalSampler, 0);

    vec4 n[9];
    make_kernel(n, normalSampler, inUV + uv_offset);

    vec4 sobel_edge_h = n[2] + (2.0*n[5]) + n[8] - (n[0] + (2.0*n[3]) + n[6]);
    vec4 sobel_edge_v = n[0] + (2.0*n[1]) + n[2] - (n[6] + (2.0*n[7]) + n[8]);
    vec4 sobel = sqrt((sobel_edge_h * sobel_edge_h) + (sobel_edge_v * sobel_edge_v));
    // The prominent edge will be bright R
    if (sobel.r > SOBEL_THRESHOLD || sobel.g > SOBEL_THRESHOLD || sobel.b > SOBEL_THRESHOLD) {
        outColor = SOBEL_OUTLINE_COLOR;
    } else {
        vec3 fragPos = texture(positionSampler, inUV).rgb;
        vec3 viewDir = normalize(ubo.camPos.xyz - fragPos);
        vec3 normal = texture(normalSampler, inUV).rgb;
//        vec2 objUv = vec2(texture(normalSampler, inUV).a, texture(positionSampler, inUV).a);

        // Skip for sky
        if (normal == vec3(0,0,0)) {
            outColor = texture(colorSampler, inUV);
            return;
        }

        float brightness = 0;
        bool shouldShadeBlack = true;
        for (int i = -1; i < LIGHT_COUNT; ++i) {
            // early stop condition
            if (i != -1 && ubo.lights[i].colorAndRadius.rgb == vec3(0, 0, 0)) {
                break;
            }
            // TODO: check for light distance

            // brightness to determine line shading
            vec3 lightDir;
            if (i >= 0) { lightDir = normalize(ubo.lights[i].position.xyz - fragPos); }
            else { lightDir = -ubo.globalDirLight.direction.xyz; }
            vec3 halfwayDir = normalize(lightDir + viewDir);
            // ambient specular
            brightness += max(dot(normal, lightDir), 0.0);
            brightness += SPEC_STRENGTH * pow(max(dot(normal, halfwayDir), 0.0), SPEC_SHININESS);
        }

        // TODO: to make it more believable, mesh should stay the same based on rotation

        if (shouldProdecuralMobius(inUV * textureSize(colorSampler, 0), brightness, noiseVal)) {
            outColor = vec4(0, 0, 0, 0);
        } else {
            outColor = clamp(brightness, 0.4, 1) * texture(colorSampler, inUV);
        }
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

//    // https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
//    lighting = clamp(lighting, 0, 1);
//    float luminance = (0.299*lighting.r + 0.587*lighting.g + 0.114*lighting.b);
//
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
//    drawBlinnPhong();

    // NOTE: gamma correction is already being handled by SRGB frambuffer,
    // https://learnopengl.com/Advanced-Lighting/Gamma-Correction
}

