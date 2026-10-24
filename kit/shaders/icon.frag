// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// Tints a white vector icon rendered by Qt SVG: the source alpha becomes the
// coverage of a solid color, so one rasterised icon serves every theme.
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 tint; // straight RGBA
};

layout(binding = 1) uniform sampler2D source;

void main()
{
    float coverage = texture(source, qt_TexCoord0).a;
    fragColor = vec4(tint.rgb * tint.a, tint.a) * coverage * qt_Opacity;
}
