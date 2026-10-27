// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// Lumen Glass: samples a shared, pre-blurred backdrop texture at this item's
// exact on-screen rectangle, tints it, adds blue-noise grain against banding,
// and composes the same SDF shape, hairline and shadow as Surface.
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 effectSize;
    vec2 shapeSize;
    float radius;
    float exponent;
    vec4 backdropRect;   // effect rect in normalized backdrop coordinates (x, y, w, h)
    float backdropMix;   // 1 when a backdrop is bound, 0 for a tint-only fallback
    vec4 tint;           // straight RGBA; alpha = tint strength over the backdrop
    float grain;
    vec4 strokeColor;
    float strokeWidth;
    vec4 shadowColor;
    float shadowBlur;
    float shadowOffsetY;
    float highlight;
};

layout(binding = 1) uniform sampler2D backdropTex;

float sdSquircleRect(vec2 p, vec2 halfSize, float r, float n)
{
    vec2 q = abs(p) - halfSize + vec2(r);
    vec2 qc = max(q, vec2(0.0));
    float corner = r > 0.0 ? pow(pow(qc.x, n) + pow(qc.y, n), 1.0 / n) : length(qc);
    return corner + min(max(q.x, q.y), 0.0) - r;
}

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main()
{
    vec2 p = qt_TexCoord0 * effectSize - 0.5 * effectSize;
    vec2 halfSize = 0.5 * shapeSize;

    float d = sdSquircleRect(p, halfSize, radius, exponent);
    float aa = max(fwidth(d), 1e-3);
    float coverage = clamp(0.5 - d / aa, 0.0, 1.0);
    float v = clamp((p.y + halfSize.y) / max(shapeSize.y, 1.0), 0.0, 1.0);

    vec2 uv = clamp(backdropRect.xy + qt_TexCoord0 * backdropRect.zw, vec2(0.0), vec2(1.0));
    vec3 behind = texture(backdropTex, uv).rgb;
    vec3 tinted = mix(behind, tint.rgb, tint.a);
    vec3 base = mix(tint.rgb, tinted, backdropMix);

    float sheen = highlight * 0.06 * (1.0 - smoothstep(0.0, min(40.0, shapeSize.y * 0.5), p.y + halfSize.y));
    base += vec3(sheen);
    base += (hash12(qt_TexCoord0 * effectSize * 2.0) - 0.5) * grain;

    vec4 color = vec4(base, 1.0) * coverage;

    if (strokeWidth > 0.0 && strokeColor.a > 0.0) {
        float inner = clamp(0.5 - (d + strokeWidth) / aa, 0.0, 1.0);
        float ring = max(coverage - inner, 0.0);
        float specular = mix(1.0, 0.35, smoothstep(0.0, 0.55, v));
        vec4 stroke = vec4(strokeColor.rgb * strokeColor.a, strokeColor.a) * ring * specular;
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
