#ifndef ZX4PIXELIZER_H
#define ZX4PIXELIZER_H

#include <QVector>
#include <QColor>
#include <QMap>
#include <QImage>

enum ZxColorEnum {ZxBlack = 0, ZxBlue, ZxRed, ZxMagenta, ZxGreen, ZxCyan, ZxYellow, ZxWhite, ZxBrightBlack, ZxBrightBlue, ZxBrightRed, ZxBrightMagenta, ZxBrightGreen, ZxBrightCyan, ZxBrightYellow, ZxBrightWhite};

struct Zx4Image
{
    enum Bit {BitOff = 0, BitOn = 1};
    typedef QMap<int, QMap<int, Bit>> Layer; // x, y, bit
    Layer color_layer0, color_layer1, alpha_layer;
    virtual ~Zx4Image() {}
    bool isNull() const { return h <= 0 || w <= 0; }
    virtual void resize(int _w, int _h)
    {
        w = _w;
        h = _h;
        Layer _color_layer0, _color_layer1, _alpha_layer;
        for (int x = 0; x < w; x++)
        {
            for (int y = 0; y < h; y++)
            {
                _color_layer0[x][y] = color_layer0[x][y];
                _color_layer1[x][y] = color_layer1[x][y];
                _alpha_layer[x][y] = alpha_layer[x][y];
            }
        }
        color_layer0 = _color_layer0;
        color_layer1 = _color_layer1;
        alpha_layer = _alpha_layer;
    }
    int width() const { return w; }
    int height() const { return h; }
protected:
    int w = 0, h = 0;
};

struct Zx4ImageWithAttributes: public Zx4Image
{
    typedef QMap<int, QMap<int, int>> Attr; // x, y, palette index
    Attr attr; // 32x24
    virtual void resize(int _w, int _h)
    {
        Zx4Image::resize(_w, _h);
        Attr _attr;
        for (int x = 0; x < (w + 7) / 8; x++)
        {
            for (int y = 0; y < (h + 7) / 8; y++)
            {
                _attr[x][y] = attr[x][y];
            }
        }
        attr = _attr;
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
    int exactPalette(const QImage & img) const;
    QMultiMap<double, int> selectPalette(QMap<QColor, int> color_distrib4, SelectPaletteMethod method = DefaultSelectPaletteMethod) const;
    QMultiMap<double, int> selectPalette(const QImage & img, SelectPaletteMethod method = DefaultSelectPaletteMethod) const;
    QVector<QRgb> colorTable(int p_idx) const;
};

struct Zx4Processing
{
    static QImage scaleImage(QImage img, int w, int h);
    static QMultiMap<int, QColor> colorDistribution(QImage img, bool use_alpha = true);
    static bool alikeColors(QColor c1, QColor c2, int hue_precision = 20, int light_precision = 32, int satur_precision = 32);
    static QMap<QColor, int> colorDistribution4(QImage img);
    static QImage importImage(QImage img, QVector<QRgb> color_table, bool use_alpha);
    static QImage importImage(QImage small_quad, QVector<QRgb> color_table);
    static QByteArray deflateRle8(QByteArray ba);
    static QByteArray inflateRle8(QByteArray ba);
};

#endif // ZX4PIXELIZER_H
