// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// Lumen Surface: one pass renders a continuous-curvature ("squircle") shape
// with a vertical fill gradient, an inner hairline stroke with a specular
// falloff, an inner top sheen and an analytic soft shadow. All geometry is a
// signed distance field, so edges stay crisp at any scale or transform.
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 effectSize;     // ShaderEffect size in logical px (shape + 2 * shadow padding)
    vec2 shapeSize;      // shape size in logical px
    float radius;        // corner radius in logical px
    float exponent;      // 2 = circular corners, ~4 = continuous curvature
    vec4 fillTop;        // straight RGBA
    vec4 fillBottom;     // straight RGBA
    vec4 strokeColor;    // straight RGBA
    float strokeWidth;
    vec4 shadowColor;    // straight RGBA
    float shadowBlur;
    float shadowOffsetY;
    float highlight;     // inner top sheen strength 0..1
};

float sdSquircleRect(vec2 p, vec2 halfSize, float r, float n)
{
    vec2 q = abs(p) - halfSize + vec2(r);
    vec2 qc = max(q, vec2(0.0));
    float corner = r > 0.0 ? pow(pow(qc.x, n) + pow(qc.y, n), 1.0 / n) : length(qc);
    return corner + min(max(q.x, q.y), 0.0) - r;
}

vec4 premultiply(vec4 c) { return vec4(c.rgb * c.a, c.a); }

void main()
{
    vec2 p = qt_TexCoord0 * effectSize - 0.5 * effectSize;
    vec2 halfSize = 0.5 * shapeSize;

    float d = sdSquircleRect(p, halfSize, radius, exponent);
    float aa = max(fwidth(d), 1e-3);
    float coverage = clamp(0.5 - d / aa, 0.0, 1.0);

    float v = clamp((p.y + halfSize.y) / max(shapeSize.y, 1.0), 0.0, 1.0);
    vec4 color = premultiply(mix(fillTop, fillBottom, v)) * coverage;

    if (highlight > 0.0) {
        float depth = p.y + halfSize.y;
        float sheen = highlight * 0.07 * (1.0 - smoothstep(0.0, min(36.0, shapeSize.y * 0.5), depth));
        color.rgb += vec3(sheen) * coverage;
    }

    if (strokeWidth > 0.0 && strokeColor.a > 0.0) {
        float inner = clamp(0.5 - (d + strokeWidth) / aa, 0.0, 1.0);
        float ring = max(coverage - inner, 0.0);
        float specular = mix(1.0, 0.45, smoothstep(0.0, 0.6, v));
        vec4 stroke = premultiply(strokeColor) * ring * specular;
        color = stroke + color * (1.0 - stroke.a);
    }

    if (shadowColor.a > 0.0 && shadowBlur > 0.0) {
        float ds = sdSquircleRect(p - vec2(0.0, shadowOffsetY), halfSize, radius, exponent);
        float s = 1.0 - smoothstep(-0.35 * shadowBlur, shadowBlur, ds);
        s *= s;
        float alpha = shadowColor.a * s * (1.0 - coverage);
        color += vec4(shadowColor.rgb * alpha, alpha) * (1.0 - color.a);
    }

    fragColor = color * qt_Opacity;
}
