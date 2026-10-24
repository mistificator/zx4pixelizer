// http://www.easyrgb.com/index.php?X=MATH&H=02#text2
#include <math.h>
#include "color_conversions.h"
#include "CIEDE2000.h"

float ref_X = 95.047f;
float ref_Y = 100.0f;
float ref_Z = 108.883f;

namespace {
struct LabEntry {
    float l, a, b;
};

// Таблица предвычисленных значений Lab для всех комбинаций RGB (64x64x64 = 262144 цвета, шаг 4)
static LabEntry s_labLut[64][64][64];
static bool s_lutInitialized = false;

void initLabLut()
{
    if (s_lutInitialized) return;
    for (int r = 0; r < 64; ++r)
    {
        int inR = (r << 2) | (r >> 4);
        for (int g = 0; g < 64; ++g)
        {
            int inG = (g << 2) | (g >> 4);
            for (int b = 0; b < 64; ++b)
            {
                int inB = (b << 2) | (b >> 4);
                float x, y, z;
                convertRGBtoXYZ(inR, inG, inB, &x, &y, &z);
                convertXYZtoLab(x, y, z, &s_labLut[r][g][b].l, &s_labLut[r][g][b].a, &s_labLut[r][g][b].b);
            }
        }
    }
    s_lutInitialized = true;
}

inline const LabEntry & fastRgbToLab(int r, int g, int b)
{
    if (!s_lutInitialized) initLabLut();
    return s_labLut[(r >> 2) & 0x3F][(g >> 2) & 0x3F][(b >> 2) & 0x3F];
}
} // namespace

void convertRGBtoXYZ(int inR, int inG, int inB, float * outX, float * outY, float * outZ)
{
    float var_R = (inR / 255.0f);
    float var_G = (inG / 255.0f);
    float var_B = (inB / 255.0f);

    var_R = (var_R > 0.04045f) ? powf((var_R + 0.055f) / 1.055f, 2.4f) : (var_R / 12.92f);
    var_G = (var_G > 0.04045f) ? powf((var_G + 0.055f) / 1.055f, 2.4f) : (var_G / 12.92f);
    var_B = (var_B > 0.04045f) ? powf((var_B + 0.055f) / 1.055f, 2.4f) : (var_B / 12.92f);

    var_R *= 100.0f;
    var_G *= 100.0f;
    var_B *= 100.0f;

    *outX = var_R * 0.4124f + var_G * 0.3576f + var_B * 0.1805f;
    *outY = var_R * 0.2126f + var_G * 0.7152f + var_B * 0.0722f;
    *outZ = var_R * 0.0193f + var_G * 0.1192f + var_B * 0.9505f;
}

void convertXYZtoLab(float inX, float inY, float inZ, float * outL, float * outa, float * outb)
{
    float var_X = inX / ref_X;
    float var_Y = inY / ref_Y;
    float var_Z = inZ / ref_Z;

    var_X = (var_X > 0.008856f) ? cbrtf(var_X) : (7.787f * var_X) + (16.0f / 116.0f);
    var_Y = (var_Y > 0.008856f) ? cbrtf(var_Y) : (7.787f * var_Y) + (16.0f / 116.0f);
    var_Z = (var_Z > 0.008856f) ? cbrtf(var_Z) : (7.787f * var_Z) + (16.0f / 116.0f);

    *outL = (116.0f * var_Y) - 16.0f;
    *outa = 500.0f * (var_X - var_Y);
    *outb = 200.0f * (var_Y - var_Z);
}

void convertLabtoXYZ(float inL, float ina, float inb, float * outX, float * outY, float * outZ)
{
    float var_Y = (inL + 16.0f) / 116.0f;
    float var_X = (ina / 500.0f) + var_Y;
    float var_Z = var_Y - (inb / 200.0f);

    float y3 = var_Y * var_Y * var_Y;
    float x3 = var_X * var_X * var_X;
    float z3 = var_Z * var_Z * var_Z;

    var_Y = (y3 > 0.008856f) ? y3 : (var_Y - (16.0f / 116.0f)) / 7.787f;
    var_X = (x3 > 0.008856f) ? x3 : (var_X - (16.0f / 116.0f)) / 7.787f;
    var_Z = (z3 > 0.008856f) ? z3 : (var_Z - (16.0f / 116.0f)) / 7.787f;

    *outX = ref_X * var_X;
    *outY = ref_Y * var_Y;
    *outZ = ref_Z * var_Z;
}

void convertXYZtoRGB(float inX, float inY, float inZ, int * outR, int * outG, int * outB)
{
    float var_X = inX / 100.0f;
    float var_Y = inY / 100.0f;
    float var_Z = inZ / 100.0f;

    float var_R = var_X *  3.2406f + var_Y * (-1.5372f) + var_Z * (-0.4986f);
    float var_G = var_X * (-0.9689f) + var_Y *  1.8758f + var_Z *  0.0415f;
    float var_B = var_X *  0.0557f + var_Y * (-0.2040f) + var_Z *  1.0570f;

    var_R = (var_R > 0.0031308f) ? (1.055f * powf(var_R, 1.0f / 2.4f) - 0.055f) : (12.92f * var_R);
    var_G = (var_G > 0.0031308f) ? (1.055f * powf(var_G, 1.0f / 2.4f) - 0.055f) : (12.92f * var_G);
    var_B = (var_B > 0.0031308f) ? (1.055f * powf(var_B, 1.0f / 2.4f) - 0.055f) : (12.92f * var_B);

    *outR = static_cast<int>(var_R * 255.0f);
    *outG = static_cast<int>(var_G * 255.0f);
    *outB = static_cast<int>(var_B * 255.0f);
}

float Lab_color_difference_CIE76(float inL1, float ina1, float inb1, float inL2, float ina2, float inb2)
{
    float dl = inL1 - inL2;
    float da = ina1 - ina2;
    float db = inb1 - inb2;
    return sqrtf(dl * dl + da * da + db * db);
}

float RGB_color_Lab_difference_CIE76(int R1, int G1, int B1, int R2, int G2, int B2)
{
    const auto & c1 = fastRgbToLab(R1, G1, B1);
    const auto & c2 = fastRgbToLab(R2, G2, B2);
    return Lab_color_difference_CIE76(c1.l, c1.a, c1.b, c2.l, c2.a, c2.b);
}

float Lab_color_difference_CIE94(float inL1, float ina1, float inb1, float inL2, float ina2, float inb2)
{
    // Коэффициенты Graphic Arts (вместо Textiles) для корректного учёта контраста яркости
    const float Kl = 1.0f;
    const float K1 = 0.045f;
    const float K2 = 0.015f;

    float deltaL = inL1 - inL2;
    float deltaA = ina1 - ina2;
    float deltaB = inb1 - inb2;

    float c1 = sqrtf(ina1 * ina1 + inb1 * inb1);
    float c2 = sqrtf(ina2 * ina2 + inb2 * inb2);
    float deltaC = c1 - c2;

    float deltaH2 = deltaA * deltaA + deltaB * deltaB - deltaC * deltaC;
    if (deltaH2 < 0.0f) deltaH2 = 0.0f;

    float sc = 1.0f + K1 * c1;
    float sh = 1.0f + K2 * c1;

    float lTerm = deltaL / Kl;
    float cTerm = deltaC / sc;
    float hTerm2 = deltaH2 / (sh * sh);

    float i = lTerm * lTerm + cTerm * cTerm + hTerm2;
    return (i <= 0.0f) ? 0.0f : sqrtf(i);
}

float RGB_color_Lab_difference_CIE94(int R1, int G1, int B1, int R2, int G2, int B2)
{
    const auto & c1 = fastRgbToLab(R1, G1, B1);
    const auto & c2 = fastRgbToLab(R2, G2, B2);
    return Lab_color_difference_CIE94(c1.l, c1.a, c1.b, c2.l, c2.a, c2.b);
}

float Lab_color_difference_CIEDE2000(float inL1, float ina1, float inb1, float inL2, float ina2, float inb2)
{
    return CIEDE2000::CIEDE2000(CIEDE2000::LAB(inL1, ina1, inb1), CIEDE2000::LAB(inL2, ina2, inb2));
}

float RGB_color_Lab_difference_CIEDE2000(int R1, int G1, int B1, int R2, int G2, int B2)
{
    const auto & c1 = fastRgbToLab(R1, G1, B1);
    const auto & c2 = fastRgbToLab(R2, G2, B2);
    return Lab_color_difference_CIEDE2000(c1.l, c1.a, c1.b, c2.l, c2.a, c2.b);
}
