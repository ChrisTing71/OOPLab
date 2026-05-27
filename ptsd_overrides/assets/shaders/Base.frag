#version 410 core

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

uniform sampler2D surface;
uniform vec4 tintColor = vec4(1.0, 1.0, 1.0, 1.0);
uniform float fillProgress = 1.0;
uniform bool showFillProgress = false;

void main() {
    vec4 texColor = texture(surface, uv);

    if (texColor.a < 0.01)
        discard;

    if (showFillProgress) {
        // Show fill progress from bottom to top
        // uv.y goes from 0 (top) to 1 (bottom), so check if we're in the bottom portion
        if (uv.y > (1.0 - fillProgress)) {
            // In the filled region (bottom portion) - brighten the color
            fragColor = texColor * vec4(1.2, 1.2, 1.2, 1.0) * tintColor;
        } else {
            // In the unfilled region (top portion) - darken to gray
            float gray = (texColor.r + texColor.g + texColor.b) * 0.33;
            fragColor = vec4(gray * 0.5, gray * 0.5, gray * 0.5, texColor.a) * tintColor;
        }
    } else {
        fragColor = texColor * tintColor;
    }
}
