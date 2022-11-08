#ifndef ZX4PIXELIZER_H
#define ZX4PIXELIZER_H

#include <QVector>
#include <QColor>
#include <QMap>
#include <QImage>
#include <QImageReader>

enum ZxColorEnum {ZxBlack = 0, ZxBlue, ZxRed, ZxMagenta, ZxGreen, ZxCyan, ZxYellow, ZxWhite, ZxBrightBlack, ZxBrightBlue, ZxBrightRed, ZxBrightMagenta, ZxBrightGreen, ZxBrightCyan, ZxBrightYellow, ZxBrightWhite};

struct Zx4Image
{
    enum Bit {BitOff = 0, BitOn = 1};
    typedef QMap<int, QMap<int, Bit>> Layer; // x, y, bit
    Layer color_layer0, color_layer1, alpha_layer;
    virtual ~Zx4Image() {}
    virtual void resize(int _w, int _h);
    bool isNull() const { return h <= 0 || w <= 0; }
    int width() const { return w; }
    int height() const { return h; }
    virtual Zx4Image * clone() const { return new Zx4Image(* this); }
protected:
    int w = 0, h = 0;
};

struct Zx4Palette;
struct Zx4ImageWithAttributes: public Zx4Image
{
    Zx4ImageWithAttributes(int _w = 0, int _h = 0);
    Zx4ImageWithAttributes(const Zx4Image & o, int p_index);
    typedef QMap<int, QMap<int, int>> Attr; // x, y, palette index
    Attr attr; // 32x24
    virtual void resize(int _w, int _h);
    QPair<QByteArray, QByteArray> makeScreens(const Zx4Palette & palettes) const;
    void makeAttr(int p_idx);
    Zx4ImageWithAttributes & copy(const Zx4ImageWithAttributes & o, QPoint pt = QPoint(0, 0));
    virtual Zx4Image * clone() const { return new Zx4ImageWithAttributes(* this); }
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

struct Zx4ImportPars
{
    bool alpha = true;
    bool autopalette = true;
    int palette_index = -1;
    int palette_index_max = -1;

    bool multipass = false;
    Zx4Palette::SelectPaletteMethod color_pick = Zx4Palette::SelectPaletteHSL;

    bool horizontal_mirror = false;
    bool vertical_mirror = false;

    double x_shift = 0, y_shift = 0;
    double x_scale = 1, y_scale = 1;
    double rotate = 0;
    double hue_shift = 0, saturation_shift = 0, lightness_shift = 0;
};

class Zx4BaseObject
{
public:
    Zx4BaseObject(const Zx4Palette & _palettes);
    virtual ~Zx4BaseObject();

    virtual void setFileName(QString);
    QString fileName() const;

    virtual void setSourceImage(const QList<QImage> &);
    void setSourceImage(const QImage &);
    void setSourceImage(QImageReader &);
    const QImage & sourceImage(int frame = 0) const;
    QImage & transformedImage(int frame = 0);

    virtual void setImage(const QList<const Zx4Image *> &);
    void setImage(const Zx4Image &);
    const Zx4Image & image(int frame = 0) const;

    virtual void setCurrentColorIndex(int);
    int currentColorIndex() const;

    virtual void setImportPars(const Zx4ImportPars &);
    const Zx4ImportPars & importPars() const;

    QWidget * canvas() const;

    void setCurrentFrame(int frame);
    int currentFrame() const;
    int framesCount() const;
protected:
    void setCanvas(QWidget *);
    const Zx4Palette & palettes;
    QString file_name;
    mutable QList<Zx4Image *> zx4_img;
    QList<QImage> src_imp_img, transformed_img;
    int grid_x = 64, grid_y = 64;
    int current_color_index = 0;
    int current_frame = 0;
    Zx4ImportPars imp_pars;
    QWidget * widget = nullptr;
};

#endif // ZX4PIXELIZER_H
