#version 450

layout(binding = 0) uniform sampler2D frame;

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

const int blurCount = 8;

void main() {
    vec2 texelSize = 1.0 / textureSize(frame, 0);
    vec3 color = vec3(0.0);
    for(int x = -blurCount; x <= blurCount; x++) {
        for(int y = -blurCount; y <= blurCount; y++) {
            color += texture(frame, inTexCoord + vec2(x, y) * texelSize).rgb;
        }
    }
    color /= float((2*blurCount+1)*(2*blurCount+1));
    outColor = vec4(color, 1.0);
}
