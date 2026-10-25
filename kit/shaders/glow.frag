// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// The audio-reactive light over the Aurora wallpaper: three soft blooms in
// the accent's neighbourhood that swell with the music's energy and flare on
// beats. Cheap (a few exponentials per pixel) and drawn only while there is
// something to react to; the wallpaper's noise field underneath stays cached.
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
    float energy;      // 0..1
    float pulse;       // 0..1
};

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

vec3 oklchToSrgb(float l, float c, float hDeg)
{
    float h = radians(hDeg);
    vec3 linear = clamp(oklabToLinearSrgb(vec3(l, c * cos(h), c * sin(h))), 0.0, 1.0);
    vec3 lo = 12.92 * linear;
    vec3 hi = 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055;
    return mix(lo, hi, step(vec3(0.0031308), linear));
}

float bloom(vec2 p, vec2 centre, float radius)
{
    vec2 d = p - centre;
    return exp(-dot(d, d) / (radius * radius));
}

void main()
{
    float aspect = resolution.x / max(resolution.y, 1.0);
    vec2 p = vec2(qt_TexCoord0.x * aspect, qt_TexCoord0.y);
    float t = time * 0.07;

    // Light rising from the bottom corners, swelling with the energy; a
    // third bloom low in the centre flares on beats.
    float swell = 0.20 + 0.14 * energy + 0.06 * pulse;
    vec2 a = vec2(aspect * (0.16 + 0.05 * sin(t * 1.3)), 1.08 + 0.03 * cos(t));
    vec2 b = vec2(aspect * (0.84 + 0.05 * cos(t * 0.9)), 1.06 + 0.03 * sin(t * 1.1));
    vec2 c = vec2(aspect * (0.50 + 0.10 * sin(t * 0.7)), 1.00 + 0.03 * cos(t * 1.7));

    float wa = bloom(p, a, swell * 1.2);
    float wb = bloom(p, b, swell * 1.1);
    float wc = bloom(p, c, swell * 0.9) * pulse;

    float lightness = mix(0.80, 0.72, darkness);
    vec3 colour = oklchToSrgb(lightness, 0.17, hue + 28.0) * wa
                + oklchToSrgb(lightness, 0.17, hue - 32.0) * wb
                + oklchToSrgb(lightness + 0.06, 0.12, hue) * wc;

    float strength = 0.45 * energy + 0.40 * pulse;
    // Additive light in the dark; a soft tint over light wallpapers.
    float coverage = clamp(wa + wb + wc, 0.0, 1.0) * strength;
    vec3 light = colour * strength * mix(0.45, 0.75, darkness);
    fragColor = vec4(light, coverage * mix(0.35, 0.0, darkness)) * qt_Opacity;
}
