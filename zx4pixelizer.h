#ifndef ZX4PIXELIZER_H
#define ZX4PIXELIZER_H

#include <QVector>
#include <QList>
#include <QColor>
#include <QMap>
#include <QMultiMap>
#include <QPair>
#include <QImage>
#include <QImageReader>
#include <QCursor>
#include <QSharedPointer>
#include <QHashFunctions>
#include <functional>

enum ZxColorEnum {
    ZxBlack = 0, ZxBlue, ZxRed, ZxMagenta, ZxGreen, ZxCyan, ZxYellow, ZxWhite,
    ZxBrightBlack, ZxBrightBlue, ZxBrightRed, ZxBrightMagenta, ZxBrightGreen, ZxBrightCyan, ZxBrightYellow, ZxBrightWhite
};

enum Zx4CursorType {
    CursorPencil = 0,
    CursorBrush,
    CursorFill,
    CursorEraser,
    CursorMove,
    CursorPipette,
    CursorStamp
};

inline bool operator<(const QColor &t1, const QColor &t2) noexcept
{
    return t1.rgba() < t2.rgba();
}

inline size_t qHash(const QColor &t, size_t seed = 0) noexcept
{
    return ::qHash(t.rgba(), seed);
}

struct Zx4Image
{
    enum Bit {BitOff = 0, BitOn = 1};
    typedef QMap<int, QMap<int, Bit>> Layer; // x, y, bit
    Layer color_layer0, color_layer1, alpha_layer;

    virtual ~Zx4Image() = default;
    virtual void resize(int _w, int _h);
    virtual void clear(int _w, int _h);
    bool isNull() const { return h <= 0 || w <= 0; }
    int width() const { return w; }
    int height() const { return h; }
    virtual Zx4Image * clone() const { return new Zx4Image(*this); }

protected:
    int w = 0, h = 0;
};

struct Zx4Palette;

struct Zx4ImageWithAttributes : public Zx4Image
{
    Zx4ImageWithAttributes(int _w = 0, int _h = 0);
    Zx4ImageWithAttributes(const Zx4Image & o, int p_index);
    typedef QMap<int, QMap<int, int>> Attr; // x, y, palette index
    Attr attr; // 32x24

    void resize(int _w, int _h) override;
    QPair<QByteArray, QByteArray> makeScreens(const Zx4Palette & palettes) const;
    void makeAttr(int p_idx);
    Zx4ImageWithAttributes & copy(const Zx4ImageWithAttributes & o, QPoint pt = QPoint(0, 0));
    Zx4Image * clone() const override { return new Zx4ImageWithAttributes(*this); }
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
    int grayscale_index = -1;

    Zx4Palette() = default;
    void init();

    enum SelectPaletteMethod {
        SelectPaletteHSL = 0,
        SelectPaletteCIE76,
        SelectPaletteCIE94,
        SelectPaletteCIEDE2000,
        DefaultSelectPaletteMethod = SelectPaletteHSL
    };

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
    enum PassMethod {SinglePass = 0, Multipass5x5x2, Multipass7x7x3, Multipass3x3x1, Multipass5x5x1, Multipass7x7x1};
    PassMethod multipass = SinglePass;
    Zx4Palette::SelectPaletteMethod color_pick = Zx4Palette::SelectPaletteHSL;
    bool horizontal_mirror = false;
    bool vertical_mirror = false;
    double x_shift = 0, y_shift = 0;
    double x_scale = 1, y_scale = 1;
    double rotate = 0;
    double hue_shift = 0, saturation_shift = 0, lightness_shift = 0;

    QByteArray serialize() const;
    bool deserialize(const QByteArray & ba);
    static QString defaultPresetPath();
};

class Zx4BaseObject
{
public:
    Zx4BaseObject(const Zx4Palette & _palettes);
    virtual ~Zx4BaseObject();

    static QCursor makeCursor(Zx4CursorType type);

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

    void setAutoPalette(bool);
    bool autoPalette() const;
    void setPaletteSequenceIndex(int index, int max_index);
    int paletteSequenceIndex() const;
    int paletteSequenceMax() const;

    QWidget * canvas() const;

    void setCurrentFrame(int frame);
    int currentFrame() const;
    int framesCount() const;

    void storeToHistory(QString desc);
    void clearHistory();
    void undo();
    void redo();
    bool canUndo(QString * desc = nullptr);
    bool canRedo(QString * desc = nullptr);
    QString historyCurrentDescription() const;
    std::function<void()> on_change = nullptr;

protected:
    void setCanvas(QWidget *);
    virtual void onHistoryRestored() {}

    const Zx4Palette & palettes;
    QString file_name;
    mutable QList<QSharedPointer<Zx4Image>> zx4_img;
    QPair<int, QList<QPair<QString, QList<QSharedPointer<Zx4Image>>>>> history;
    QList<QImage> src_imp_img, transformed_img;
    int grid_x = 64, grid_y = 64;
    int current_color_index = 0;
    int current_frame = 0;
    bool auto_palette = true;
    int palette_seq_index = 1;
    int palette_seq_max = 1;
    Zx4ImportPars imp_pars;
    QWidget * widget = nullptr;
};

#endif // ZX4PIXELIZER_H
