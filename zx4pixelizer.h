#ifndef ZX4PIXELIZER_H
#define ZX4PIXELIZER_H

#include <QVector>
#include <QColor>
#include <QMap>

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

template <> inline bool qMapLessThanKey<QColor>(const QColor &key1, const QColor &key2)
{
    return key1.rgb() < key2.rgb();
}

template <> inline uint qHash<QColor>(const QColor &t, uint seed)
    noexcept(noexcept(qHash(t.rgba())))
{
    return qHash(t.rgba()) ^ seed;
}

template <> inline bool operator< <QColor, QColor>(const QPair<QColor, QColor> &p1, const QPair<QColor, QColor> &p2)
{
    return p1.first.rgba() < p2.first.rgba() || (!(p2.first.rgba() < p1.first.rgba()) && p1.second.rgba() < p2.second.rgba());
}

template <>
class qLess<QColor>
{
public:
    inline bool operator()(const QColor &t1, const QColor &t2) const
    {
        return (t1.rgba() < t2.rgba());
    }
};

struct Zx4Palette
{
    QMap<ZxColorEnum, QColor> color_table;
    QMap<ZxColorEnum, QString> color_name;
    QMap<int, QList<ZxColorEnum>> color_palette_table;

    QList<QList<QColor>> palettes;
    QMap<QColor, QList<int>> palettes_by_color;
    QMap<QPair<QColor, QColor>, QList<int>> palettes_by_2colors;
    QMap<QPair<QColor, QColor>, QMap<QColor, QList<int>>> palettes_by_3colors;
    QMap<QPair<QColor, QColor>, QMap<QPair<QColor, QColor>, QList<int>>> palettes_by_4colors;

    void init();
};

#endif // ZX4PIXELIZER_H
