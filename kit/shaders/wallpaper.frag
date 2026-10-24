// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// "Aurora" — the default QTOS wallpaper. Domain-warped fractal noise drives a
// palette that is built and blended in OKLab, so the accent hue can rotate and
// light/dark can morph without banding or muddy transitions. Static unless
// `time` changes, so it costs one frame when nothing animates.
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 resolution;   // item size in px
    float time;        // seconds
    float hue;         // accent hue in degrees
    float darkness;    // 0 = light, 1 = dark
    vec2 parallax;     // offset in normalized units
    float intensity;   // chroma multiplier 0..1
    float zoom;        // 1 = default framing
};

vec3 oklch(float l, float c, float hDeg)
{
    float h = radians(hDeg);
    return vec3(l, c * cos(h), c * sin(h));
}

vec3 oklabToLinearSrgb(vec3 lab)
{
    float l_ = lab.x + 0.3963377774 * lab.y + 0.2158037573 * lab.z;
    float m_ = lab.x - 0.1055613458 * lab.y - 0.0638541728 * lab.z;
    float s_ = lab.x - 0.0894841775 * lab.y - 1.2914855480 * lab.z;
    vec3 lms = vec3(l_ * l_ * l_, m_ * m_ * m_, s_ * s_ * s_);
    return vec3(
         4.0767416621 * lms.x - 3.3077115913 * lms.y + 0.2309699292 * lms.z,
        -1.2684380046 * lms.x + 2.6097574011 * lms.y - 0.3413193965 * lms.z,
        -0.0041960863 * lms.x - 0.7034186147 * lms.y + 1.7076147010 * lms.z);
}

vec3 linearToSrgb(vec3 c)
{
    c = clamp(c, 0.0, 1.0);
    vec3 lo = 12.92 * c;
    vec3 hi = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
    return mix(lo, hi, step(vec3(0.0031308), c));
}

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p)
{
    const mat2 rot = mat2(0.80, 0.60, -0.60, 0.80);
    float sum = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 5; ++i) {
        sum += amp * noise(p);
        p = rot * p * 2.02 + vec2(1.7, 9.2);
        amp *= 0.5;
    }
    return sum / 0.96875;
}

void main()
{
    vec2 frag = qt_TexCoord0 * resolution;
    vec2 uv = (frag - 0.5 * resolution) / resolution.y;
    uv = uv / zoom + parallax;
    float t = time * 0.035;

    // Low-frequency, strongly warped field: large soft forms, no smoke.
    vec2 p = uv * 0.85;
    vec2 q = vec2(fbm(p + vec2(0.0, t)), fbm(p + vec2(5.2, 1.3) - t));
    vec2 r = vec2(fbm(p + 1.7 * q + vec2(1.7, 9.2) + 0.5 * t),
                  fbm(p + 1.7 * q + vec2(8.3, 2.8) - 0.3 * t));
    float f = fbm(p + 1.5 * r);

    float k = intensity;
    vec3 base = oklch(mix(0.935, 0.130, darkness), mix(0.022, 0.040, darkness), hue + 12.0);
    vec3 deep = oklch(mix(0.885, 0.195, darkness), mix(0.055, 0.085, darkness) * k, hue - 18.0);
    vec3 glow = oklch(mix(0.790, 0.600, darkness), mix(0.120, 0.175, darkness) * k, hue);
    vec3 warm = oklch(mix(0.870, 0.540, darkness), mix(0.100, 0.160, darkness) * k, hue + 60.0);
    vec3 cool = oklch(mix(0.900, 0.400, darkness), mix(0.080, 0.130, darkness) * k, hue - 62.0);

    // Aurora ribbons: two soft bands that bend with the noise field.
    float bend = (f - 0.5) * 0.9 + 0.22 * sin(uv.x * 1.7 + t * 2.0);
    float ribbonA = exp(-pow((uv.y + 0.08 - bend * 0.55) * 3.0, 2.0));
    float ribbonB = exp(-pow((uv.y - 0.30 - bend * 0.35 + 0.18 * uv.x) * 4.2, 2.0));

    vec3 lab = base;
    lab = mix(lab, deep, smoothstep(0.20, 0.90, length(q)) * 0.70);
    lab = mix(lab, cool, smoothstep(0.35, 0.95, r.y) * 0.45);
    lab = mix(lab, glow, clamp(ribbonA * (0.50 + 0.70 * f), 0.0, 1.0) * 0.90);
    lab = mix(lab, warm, clamp(ribbonB * (0.35 + 0.80 * r.x), 0.0, 1.0) * 0.70);

    // Bloom along the ribbons and fine silk folds inside them.
    lab.x += (ribbonA * 0.05 + ribbonB * 0.03) * k * mix(0.6, 1.0, darkness);
    float ridge = pow(clamp(1.0 - abs(f - 0.58) * 3.2, 0.0, 1.0), 6.0) * ribbonA;
    lab.x += ridge * mix(0.03, 0.07, darkness) * k;

    // Gentle vignette in perceptual lightness.
    float vig = smoothstep(1.35, 0.15, length(uv * vec2(0.85, 1.15)));
    lab.x *= mix(mix(0.95, 0.70, darkness), 1.0, vig);

    vec3 rgb = linearToSrgb(oklabToLinearSrgb(lab));
    rgb += (hash12(frag + vec2(17.0, 31.0)) - 0.5) / 255.0;
    fragColor = vec4(rgb, 1.0) * qt_Opacity;
}
