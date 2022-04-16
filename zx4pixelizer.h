#ifndef ZX4PIXELIZER_H
#define ZX4PIXELIZER_H

#include <QVector>
#include <QColor>
#include <QMap>
#include <QImage>

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
    int width() const { return color_layer0.isEmpty() ? 0 : color_layer0[0].count(); }
    int height() const { return color_layer0.count(); }
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
    const QMap<ZxColorEnum, QColor> color_table;
    const QMap<ZxColorEnum, QString> color_name;
    const QMap<int, QList<ZxColorEnum>> color_palette_table;

    const QList<QList<QColor>> palettes;
    const QMap<QColor, QList<int>> palettes_by_color;
    const QMap<QPair<QColor, QColor>, QList<int>> palettes_by_2colors;
    const QMap<QPair<QColor, QColor>, QMap<QColor, QList<int>>> palettes_by_3colors;
    const QMap<QPair<QColor, QColor>, QMap<QPair<QColor, QColor>, QList<int>>> palettes_by_4colors;

    const int grayscale_index = -1;

    Zx4Palette();
    void init();

    enum SelectPaletteMethod {SelectPaletteHSL = 0, SelectPaletteCIE76, SelectPaletteCIE94, SelectPaletteCIEDE2000, DefaultSelectPaletteMethod = SelectPaletteHSL};
    QMultiMap<double, int> selectPalette(QMap<QColor, int> color_distrib4, SelectPaletteMethod method = DefaultSelectPaletteMethod) const;
    QMultiMap<double, int> selectPalette(QImage img, SelectPaletteMethod method = DefaultSelectPaletteMethod) const;
};

struct Zx4Processing
{
    static QImage scaleImage(QImage img, int w, int h);
    static QMultiMap<int, QColor> colorDistribution(QImage img, bool use_alpha = true);
    static bool alikeColors(QColor c1, QColor c2, int hue_precision = 20, int light_precision = 32, int satur_precision = 32);
    static QMap<QColor, int> colorDistribution4(QImage img);
    static QImage importImage(QImage img, QVector<QRgb> color_table, bool use_alpha);
    static QByteArray deflateRle8(QByteArray ba);
    static QByteArray inflateRle8(QByteArray ba);
};

#endif // ZX4PIXELIZER_H
