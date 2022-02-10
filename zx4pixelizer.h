#ifndef ZX4PIXELIZER_H
#define ZX4PIXELIZER_H

#include <QVector>

enum ZxColorEnum {ZxBlack = 0, ZxBlue, ZxRed, ZxMagenta, ZxGreen, ZxCyan, ZxYellow, ZxWhite, ZxBrightBlack, ZxBrightBlue, ZxBrightRed, ZxBrightMagenta, ZxBrightGreen, ZxBrightCyan, ZxBrightYellow, ZxBrightWhite};

struct Zx4Image
{
    QVector<QVector<char>> color_layer0, color_layer1, alpha_layer;
    bool isNull() const { return color_layer0.isEmpty() || color_layer1.isEmpty() || alpha_layer.isEmpty(); }
    void resize(int w, int h)
    {
        color_layer0.resize(h);
        color_layer1.resize(h);
        alpha_layer.resize(h);
        for (int y = 0; y < h; ++y)
        {
            color_layer0[y].resize(w);
            color_layer1[y].resize(w);
            alpha_layer[y].resize(w);
        }
    }
};


#endif // ZX4PIXELIZER_H
