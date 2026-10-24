// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// Clips a layer (for example a whole window) to a continuous-curvature
// rounded rectangle with analytic anti-aliasing. Used as `layer.effect`.
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 size;
    float radius;
    float exponent;
};

layout(binding = 1) uniform sampler2D source;

float sdSquircleRect(vec2 p, vec2 halfSize, float r, float n)
{
    vec2 q = abs(p) - halfSize + vec2(r);
    vec2 qc = max(q, vec2(0.0));
    float corner = r > 0.0 ? pow(pow(qc.x, n) + pow(qc.y, n), 1.0 / n) : length(qc);
    return corner + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
    vec2 p = qt_TexCoord0 * size - 0.5 * size;
    float d = sdSquircleRect(p, 0.5 * size, radius, exponent);
    float coverage = clamp(0.5 - d / max(fwidth(d), 1e-3), 0.0, 1.0);
    fragColor = texture(source, qt_TexCoord0) * coverage * qt_Opacity;
}
