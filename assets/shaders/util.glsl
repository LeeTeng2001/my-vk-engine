#define PI 3.14159265
#define LINE_WIDTH 1.5
#define LINE_SEPERATION_WIDTH 10.0

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// smooth wiggle version, super handy!
float smoothwiggle(float t, float frequency, float seed) {
    t *= frequency;
    float a = rand(vec2(floor(t), seed)) * 2.0 - 1.0;
    float b = rand(vec2(ceil(t), seed)) * 2.0 - 1.0;

    t -= floor(t);

    return mix(a, b, sin(t * t * PI / 2.0)); // fake smooth blend
}

bool shouldProdecuralMobius(vec2 screenUv, float lightPerc, float noise) {
    // fragCoord += SOBEL_WIGGLE_FACTOR * smoothwiggle(uv.x + uv.y, 50.0, 128.0);
    vec2 positionInBlock = mod(screenUv + noise, LINE_WIDTH + LINE_SEPERATION_WIDTH);

    if (lightPerc < 0.15) { // darkest, calculate distance to diag
        float distanceToDiagLine = mod(abs(screenUv.x + screenUv.y), LINE_WIDTH + LINE_SEPERATION_WIDTH / 2.5);
        if (positionInBlock.x < LINE_WIDTH || positionInBlock.y < LINE_WIDTH || distanceToDiagLine < LINE_WIDTH) return true;
        return false;
    } else if (lightPerc < 0.4) { // hor, vert
        if (positionInBlock.x < LINE_WIDTH || positionInBlock.y < LINE_WIDTH) return true;
        return false;
    } else if (lightPerc < 0.6) { // vert
        if (positionInBlock.y < LINE_WIDTH) return true;
        return false;
    } else { // brightest, ignore Z
        return false;
    }
}
