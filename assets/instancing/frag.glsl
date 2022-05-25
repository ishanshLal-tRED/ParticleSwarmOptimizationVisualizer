#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragUV;

// binding 0 used for transformation data
layout (binding = 1) uniform sampler2D textureSampler;
layout (binding = 2) uniform sampler2D atlasSampler;

layout (location = 0) out vec4 outColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}
float pxRange = 2;
float screenPxRange() {
    vec2 unitRange = vec2 (pxRange)/vec2 (textureSize(atlasSampler, 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(fragUV.xy);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}
void main() {
    int texID = int (fragUV.z);
    switch (texID) {
    case 1: {
            outColor = texture(textureSampler, fragUV.xy) * vec4(fragColor, 1.0);
        } break;
    case 2: {
            vec4 msd = texture(atlasSampler, fragUV.xy);
            float sd = median(msd.r, msd.g, msd.b);
            float screenPxDistance = screenPxRange()*(sd - 0.5);
            float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
            vec3 bgColor = vec3 (0);
            outColor = vec4(mix(bgColor, fragColor, opacity), msd.a);
        } break;
    case 0: 
    default:
        outColor = vec4(fragColor, 1.0);
    }
    
}
