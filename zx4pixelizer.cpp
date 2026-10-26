#include <QSet>
#include <QHash>
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QDir>
#include <QDebug>
#include <algorithm>
#include <cstring>
#include <cmath>
#include "zx4pixelizer.h"
#include "color_conversions.h"

void Zx4Image::resize(int _w, int _h)
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

void Zx4Image::clear(int _w, int _h)
{
    w = _w;
    h = _h;
    color_layer0.clear();
    color_layer1.clear();
    alpha_layer.clear();
    for (int x = 0; x < w; x++)
    {
        for (int y = 0; y < h; y++)
        {
            color_layer0[x][y] = BitOff;
            color_layer1[x][y] = BitOff;
            alpha_layer[x][y] = BitOff;
        }
    }
}

Zx4ImageWithAttributes::Zx4ImageWithAttributes(int _w, int _h) : Zx4Image()
{
    resize(_w, _h);
}

Zx4ImageWithAttributes::Zx4ImageWithAttributes(const Zx4Image & o, int p_index) : Zx4Image(o)
{
    makeAttr(p_index);
}

void Zx4ImageWithAttributes::resize(int _w, int _h)
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

#define GetY(y)         (((y & 0xc0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2))
#define GetXY(x, y)     (GetY(y) | (x & 0x1F))

QPair<QByteArray, QByteArray> Zx4ImageWithAttributes::makeScreens(const Zx4Palette & palettes) const
{
    const int scr_size = 192 * 32;
    QByteArray scr0(scr_size + 32 * 24, 0);
    QByteArray scr1 = scr0;
    for (int y = 0; y < height(); y++)
    {
        for (int x = 0; x < width() / 8; x++)
        {
            int xy = GetXY(x, y);
            for (int bit = 0; bit < 8; bit++)
            {
                scr0[xy] = static_cast<char>(scr0[xy] | (static_cast<int>(color_layer0[(8 * x) + (7 - bit)][y]) << bit));
                scr1[xy] = static_cast<char>(scr1[xy] | (static_cast<int>(color_layer1[(8 * x) + (7 - bit)][y]) << bit));
            }
        }
    }
    for (int y = 0; y < (height() + 7) / 8; y++)
    {
        for (int x = 0; x < width() / 8; x++)
        {
            int p_idx = attr[x][y];
            QList<ZxColorEnum> z = palettes.color_palette_table[p_idx];
            scr0[scr_size + y * 32 + x] = static_cast<char>((z[1] & 0x07) | ((z[0] & 0x0F) << 3));
            scr1[scr_size + y * 32 + x] = static_cast<char>((z[3] & 0x07) | ((z[2] & 0x0F) << 3));
        }
    }
    return QPair<QByteArray, QByteArray>(scr0, scr1);
}

void Zx4ImageWithAttributes::makeAttr(int p_idx)
{
    for (int x = 0; x < (w + 7) / 8; x++)
    {
        for (int y = 0; y < (h + 7) / 8; y++)
        {
            attr[x][y] = p_idx;
        }
    }
}

Zx4ImageWithAttributes & Zx4ImageWithAttributes::copy(const Zx4ImageWithAttributes & o, QPoint pt)
{
    for (int x = pt.x(), ox = 0; x < qMin(w, pt.x() + o.w); x++, ox++)
    {
        for (int y = pt.y(), oy = 0; y < qMin(h, pt.y() + o.h); y++, oy++)
        {
            if (o.alpha_layer[ox][oy])
            {
                color_layer0[x][y] = o.color_layer0[ox][oy];
                color_layer1[x][y] = o.color_layer1[ox][oy];
                alpha_layer[x][y] = o.alpha_layer[ox][oy];
            }
        }
    }
    for (int x = pt.x() / 8, ox = 0; x < (qMin(w, pt.x() + o.w) + 7) / 8; x++, ox++)
    {
        for (int y = pt.y() / 8, oy = 0; y < (qMin(h, pt.y() + o.h) + 7) / 8; y++, oy++)
        {
            attr[x][y] = o.attr[ox][oy];
        }
    }
    return *this;
}

QByteArray Zx4ImportPars::serialize() const
{
    QByteArray ba(sizeof(Zx4ImportPars), '\0');
    std::memcpy(ba.data(), this, ba.size());
    return ba;
}

bool Zx4ImportPars::deserialize(const QByteArray & ba)
{
    if (ba.size() != sizeof(Zx4ImportPars))
    {
        return false;
    }
    std::memcpy(this, ba.constData(), ba.size());
    return true;
}

QString Zx4ImportPars::defaultPresetPath()
{
    return QDir::toNativeSeparators(QDir::homePath() + "/." + qApp->applicationName() + "/presets/");
}

void Zx4Palette::init()
{
    RGB_color_Lab_difference_CIE76(0, 0, 0, 0, 0, 0);

    const int level = 0xC0;
    const int high_level = 0xFF;
    color_table[ZxBlack] =          QColor(0x00, 0x00, 0x00);
    color_table[ZxBlue] =           QColor(0x00, 0x00, level);
    color_table[ZxRed] =            QColor(level, 0x00, 0x00);
    color_table[ZxMagenta] =        QColor(level, 0x00, level);
    color_table[ZxGreen] =          QColor(0x00, level, 0x00);
    color_table[ZxCyan] =           QColor(0x00, level, level);
    color_table[ZxYellow] =         QColor(level, level, 0x00);
    color_table[ZxWhite] =          QColor(level, level, level);
    color_table[ZxBrightBlack] =    QColor(0x00, 0x00, 0x00);
    color_table[ZxBrightBlue] =     QColor(0x00, 0x00, high_level);
    color_table[ZxBrightRed] =      QColor(high_level, 0x00, 0x00);
    color_table[ZxBrightMagenta] =  QColor(high_level, 0x00, high_level);
    color_table[ZxBrightGreen] =    QColor(0x00, high_level, 0x00);
    color_table[ZxBrightCyan] =     QColor(0x00, high_level, high_level);
    color_table[ZxBrightYellow] =   QColor(high_level, high_level, 0x00);
    color_table[ZxBrightWhite] =    QColor(high_level, high_level, high_level);
    grayscale_index = 486;

    color_name[ZxBlack] =          "Black";
    color_name[ZxBlue] =           "Blue";
    color_name[ZxRed] =            "Red";
    color_name[ZxMagenta] =        "Magenta";
    color_name[ZxGreen] =          "Green";
    color_name[ZxCyan] =           "Cyan";
    color_name[ZxYellow] =         "Yellow";
    color_name[ZxWhite] =          "White";
    color_name[ZxBrightBlack] =    "Bright black";
    color_name[ZxBrightBlue] =     "Bright blue";
    color_name[ZxBrightRed] =      "Bright red";
    color_name[ZxBrightMagenta] =  "Bright magenta";
    color_name[ZxBrightGreen] =    "Bright green";
    color_name[ZxBrightCyan] =     "Bright cyan";
    color_name[ZxBrightYellow] =   "Bright yellow";
    color_name[ZxBrightWhite] =    "Bright white";

    int color_num = 0;
    for (int c0i = 0; c0i < 8; c0i++)
    {
        qApp->processEvents();
        for (int c1i = c0i; c1i < 8; c1i++)
        {
            for (int c2i = c0i; c2i < 8; c2i++)
            {
                for (int c3i = c2i; c3i < 8; c3i++)
                {
                    for (int b0 = 0; b0 < 2; b0++)
                    {
                        for (int b1 = b0; b1 < 2; b1++)
                        {
                            QColor c[4];
                            ZxColorEnum z[4];
                            z[0] = static_cast<ZxColorEnum>(c0i + 8 * b0);
                            z[1] = static_cast<ZxColorEnum>(c1i + 8 * b0);
                            z[2] = static_cast<ZxColorEnum>(c2i + 8 * b1);
                            z[3] = static_cast<ZxColorEnum>(c3i + 8 * b1);
                            c[0] = color_table[z[0]];
                            c[1] = color_table[z[1]];
                            c[2] = color_table[z[2]];
                            c[3] = color_table[z[3]];

                            QColor m[4];
                            m[0] = QColor::fromRgb((c[0].red() + c[2].red()) / 2, (c[0].green() + c[2].green()) / 2, (c[0].blue() + c[2].blue()) / 2);
                            m[1] = QColor::fromRgb((c[0].red() + c[3].red()) / 2, (c[0].green() + c[3].green()) / 2, (c[0].blue() + c[3].blue()) / 2);
                            m[2] = QColor::fromRgb((c[1].red() + c[2].red()) / 2, (c[1].green() + c[2].green()) / 2, (c[1].blue() + c[2].blue()) / 2);
                            m[3] = QColor::fromRgb((c[1].red() + c[3].red()) / 2, (c[1].green() + c[3].green()) / 2, (c[1].blue() + c[3].blue()) / 2);

                            QSet<QRgb> unique_colors = { m[0].rgb(), m[1].rgb(), m[2].rgb(), m[3].rgb() };
                            if (unique_colors.size() < 4)
                            {
                                continue;
                            }

                            QList<QColor> list = { m[0], m[1], m[2], m[3] };
                            if (palettes.contains(list))
                            {
                                continue;
                            }

                            palettes << list;
                            palettes_by_color[m[0]] << color_num;
                            palettes_by_color[m[1]] << color_num;
                            palettes_by_color[m[2]] << color_num;
                            palettes_by_color[m[3]] << color_num;
                            color_palette_table[color_num] << z[0] << z[1] << z[2] << z[3];
                            color_num++;
                        }
                    }
                }
            }
        }
    }

    for (auto i = palettes_by_color.begin(); i != palettes_by_color.end(); ++i)
    {
        QSet<int> s(i.value().begin(), i.value().end());
        i.value() = s.values();
        std::sort(i.value().begin(), i.value().end());
    }

    for (auto i = palettes_by_color.cbegin(); i != palettes_by_color.cend(); ++i)
    {
        qApp->processEvents();
        QSet<int> set_i(i.value().begin(), i.value().end());
        for (auto k = i + 1; k != palettes_by_color.cend(); ++k)
        {
            QSet<int> set_k(k.value().begin(), k.value().end());
            auto list = (set_i & set_k).values();
            if (!list.isEmpty())
            {
                std::sort(list.begin(), list.end());
                palettes_by_2colors[QPair<QColor, QColor>(i.key(), k.key())] = list;
            }
        }
    }

    for (auto i = palettes_by_2colors.cbegin(); i != palettes_by_2colors.cend(); ++i)
    {
        qApp->processEvents();
        QSet<int> set_i(i.value().begin(), i.value().end());
        for (auto k = palettes_by_color.cbegin(); k != palettes_by_color.cend(); ++k)
        {
            if (k.key().rgba() <= i.key().second.rgba())
            {
                continue;
            }
            QSet<int> set_k(k.value().begin(), k.value().end());
            auto list = (set_i & set_k).values();
            if (!list.isEmpty())
            {
                std::sort(list.begin(), list.end());
                palettes_by_3colors[i.key()][k.key()] = list;
            }
        }
    }

    for (auto i = palettes_by_2colors.cbegin(); i != palettes_by_2colors.cend(); ++i)
    {
        qApp->processEvents();
        QSet<int> set_i(i.value().begin(), i.value().end());
        for (auto k = i + 1; k != palettes_by_2colors.cend(); ++k)
        {
            if (k.key().first.rgba() <= i.key().second.rgba())
            {
                continue;
            }
            QSet<int> set_k(k.value().begin(), k.value().end());
            auto list = (set_i & set_k).values();
            if (!list.isEmpty())
            {
                std::sort(list.begin(), list.end());
                palettes_by_4colors[i.key()][k.key()] = list;
            }
        }
    }
}

QMultiMap<double, int> Zx4Palette::selectPalette(QMap<QColor, int> color_distrib4, SelectPaletteMethod method) const
{
    QMultiMap<double, int> dist;
    if (color_distrib4.isEmpty())
    {
        return dist;
    }

    double total_weight = 0;
    for (auto c = color_distrib4.cbegin(); c != color_distrib4.cend(); ++c)
    {
        total_weight += c.value();
    }
    if (total_weight <= 0) total_weight = 1.0;

    for (int i = 0; i < palettes.count(); i++)
    {
        const QList<QColor> & pal = palettes[i];
        double total_error = 0.0;

        for (auto c = color_distrib4.cbegin(); c != color_distrib4.cend(); ++c)
        {
            const QColor img_c = c.key().toRgb();
            const QColor img_hsl = c.key().toHsl();
            const int c_h = img_hsl.hslHue();
            const int c_s = img_hsl.hslSaturation();
            const int c_l = img_hsl.lightness();

            double best_diff = 1e9;
            for (const QColor & p : pal)
            {
                double diff = 0.0;
                switch (method)
                {
                case SelectPaletteCIE76:
                    diff = RGB_color_Lab_difference_CIE76(p.red(), p.green(), p.blue(), img_c.red(), img_c.green(), img_c.blue());
                    break;
                case SelectPaletteCIE94:
                    diff = RGB_color_Lab_difference_CIE94(p.red(), p.green(), p.blue(), img_c.red(), img_c.green(), img_c.blue());
                    break;
                case SelectPaletteCIEDE2000:
                    diff = RGB_color_Lab_difference_CIEDE2000(p.red(), p.green(), p.blue(), img_c.red(), img_c.green(), img_c.blue());
                    break;
                default:
                {
                    QColor p_hsl = p.toHsl();
                    int p_h = p_hsl.hslHue();
                    int p_s = p_hsl.hslSaturation();
                    int p_l = p_hsl.lightness();
                    if (p_h != -1 && c_h != -1)
                    {
                        int dh = qAbs(p_h - c_h);
                        if (dh > 180) dh = 360 - dh;
                        diff = dh;
                    }
                    else if (p_h != c_h)
                    {
                        diff = 45.0;
                    }
                    diff += 0.5 * qAbs(p_s - c_s) + 0.8 * qAbs(p_l - c_l);
                    break;
                }
                }
                if (diff < best_diff)
                {
                    best_diff = diff;
                }
            }
            total_error += best_diff * c.value();
        }
        dist.insert(total_error / total_weight, i);
    }
    return dist;
}

int Zx4Palette::exactPalette(const QImage & img) const
{
    if (img.colorCount() >= 4)
    {
        QVector<QRgb> colors = img.colorTable().mid(0, 4);
        std::sort(colors.begin(), colors.end());
        QPair<QColor, QColor> firstPair(colors[0], colors[1]);
        QPair<QColor, QColor> secondPair(colors[2], colors[3]);
        if (palettes_by_4colors.contains(firstPair))
        {
            const auto & inner = palettes_by_4colors[firstPair];
            if (inner.contains(secondPair))
            {
                const QList<int> & list = inner[secondPair];
                if (!list.isEmpty())
                {
                    return list.first();
                }
            }
        }
    }
    return -1;
}

QMultiMap<double, int> Zx4Palette::selectPalette(const QImage & img, SelectPaletteMethod method) const
{
    const int p_idx = exactPalette(img);
    if (p_idx >= 0)
    {
        QMultiMap<double, int> map;
        map.insert(0, p_idx);
        return map;
    }
    return selectPalette(Zx4Processing::colorDistribution4(img), method);
}

QVector<QRgb> Zx4Palette::colorTable(int p_idx) const
{
    QVector<QRgb> color_table;
    if (p_idx < 0 || p_idx >= palettes.count())
    {
        return color_table;
    }
    for (const QColor & c : palettes[p_idx])
    {
        color_table << c.rgb();
    }
    return color_table;
}

QImage Zx4Processing::scaleImage(QImage img, int w, int h)
{
    img = img.convertToFormat(QImage::Format_ARGB32);
    QImage out(img.scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation).size(), QImage::Format_ARGB32);
    out.fill(QColor(0, 0, 0, 0));
    if (img.isNull() || out.isNull())
    {
        return out;
    }
    for (int y = 0; y < img.height(); y++)
    {
        for (int x = 0; x < img.width(); x++)
        {
            int ix = x * out.width() / img.width();
            int iy = y * out.height() / img.height();
            QColor px;
            QColor old_px_rgb = out.pixelColor(ix, iy);
            QColor new_px_rgb = img.pixelColor(x, y);
            QColor old_px_hsl = old_px_rgb.toHsl();
            QColor new_px_hsl = new_px_rgb.toHsl();
            if (new_px_hsl.hslHue() == -1 || old_px_hsl.hslHue() == -1)
            {
                px = new_px_rgb;
            }
            else
            {
                px = new_px_hsl.hslSaturation() > old_px_hsl.hslSaturation() ? new_px_hsl : old_px_hsl;
                px.setHsl(
                    px.hslHue(),
                    px.hslSaturation(),
                    px.lightness(),
                    qMax(new_px_hsl.alpha(), old_px_hsl.alpha()));
            }
            out.setPixelColor(ix, iy, px.toRgb());
        }
    }
    return out;
}

QMultiMap<int, QColor> Zx4Processing::colorDistribution(QImage img, bool use_alpha)
{
    if (img.isNull())
    {
        return {};
    }
    bool src_alpha_channel = img.hasAlphaChannel() && qAlpha(img.pixel(0, 0)) < 128;
    QRgb corner_rgb = img.pixel(0, 0) & 0x00FFFFFF;

    QMap<QRgb, int> counts;
    for (int y = 0; y < img.height(); y++)
    {
        for (int x = 0; x < img.width(); x++)
        {
            QRgb px = img.pixel(x, y);
            if (use_alpha)
            {
                if ((src_alpha_channel && qAlpha(px) < 128) ||
                    (!src_alpha_channel && (px & 0x00FFFFFF) == corner_rgb))
                {
                    continue;
                }
            }
            int r = (qRed(px) & 0xF8) | (qRed(px) >> 5);
            int g = (qGreen(px) & 0xF8) | (qGreen(px) >> 5);
            int b = (qBlue(px) & 0xF8) | (qBlue(px) >> 5);
            counts[qRgb(r, g, b)]++;
        }
    }

    QMultiMap<int, QColor> color_distrib2;
    for (auto i = counts.cbegin(); i != counts.cend(); ++i)
    {
        if (i.value() > 0)
        {
            color_distrib2.insert(i.value(), QColor::fromRgb(i.key()));
        }
    }
    return color_distrib2;
}

bool Zx4Processing::alikeColors(QColor c1, QColor c2, int hue_precision, int light_precision, int satur_precision)
{
    c1 = c1.toHsl();
    c2 = c2.toHsl();
    int c1_h = c1.hslHue();
    int c2_h = c2.hslHue();
    if (c1_h != -1 && c2_h != -1)
    {
        int _d = qAbs(c1_h - c2_h);
        if (_d > 180)
        {
            _d = 360 - _d;
        }
        if (_d > hue_precision)
        {
            return false;
        }
    }
    if (qAbs(c1.lightness() - c2.lightness()) > light_precision)
    {
        return false;
    }
    if (qAbs(c1.hslSaturation() - c2.hslSaturation()) > satur_precision)
    {
        return false;
    }
    return true;
}

QMap<QColor, int> Zx4Processing::colorDistribution4(QImage img)
{
    QMultiMap<int, QColor> color_distrib2 = colorDistribution(img);
    QMap<QColor, int> color_distrib4;
    for (auto i = color_distrib2.end(); i != color_distrib2.begin() && color_distrib4.count() < 4;)
    {
        --i;
        QColor c = i.value();
        bool merged = false;
        for (auto k = color_distrib4.begin(); k != color_distrib4.end(); ++k)
        {
            if (alikeColors(c, k.key()))
            {
                k.value() += i.key();
                merged = true;
                break;
            }
        }
        if (!merged)
        {
            color_distrib4[c] = i.key();
        }
    }
    return color_distrib4;
}

QImage Zx4Processing::importImage(QImage img, QVector<QRgb> color_table, bool use_alpha)
{
    if (img.isNull() || color_table.isEmpty())
    {
        return QImage();
    }
    bool src_alpha_channel = img.hasAlphaChannel() && qAlpha(img.pixel(0, 0)) < 128;
    QRgb corner_rgb = img.pixel(0, 0) & 0x00FFFFFF;

    QImage converted = importImage(img, color_table);
    if (use_alpha)
    {
        color_table << qRgba(0, 0, 0, 0);
        converted.setColorTable(color_table);
        for (int y = 0; y < img.height(); y++)
        {
            for (int x = 0; x < img.width(); x++)
            {
                QRgb px = img.pixel(x, y);
                if ((src_alpha_channel && qAlpha(px) < 128) ||
                    (!src_alpha_channel && (px & 0x00FFFFFF) == corner_rgb))
                {
                    converted.setPixel(x, y, 4);
                }
            }
        }
    }
    return converted;
}

QImage Zx4Processing::importImage(QImage small_quad, QVector<QRgb> color_table)
{
    QImage converted(small_quad.size(), QImage::Format_Indexed8);
    converted.setColorTable(color_table);
    const int n_colors = qMin(4, int(color_table.count()));
    for (int iy = 0; iy < converted.height(); iy++)
    {
        for (int ix = 0; ix < converted.width(); ix++)
        {
            int best_idx = 0;
            double best_diff = 1e18;
            QRgb px = small_quad.pixel(ix, iy);
            int pr = qRed(px), pg = qGreen(px), pb = qBlue(px);
            int p_max = qMax(pr, qMax(pg, pb));
            int p_min = qMin(pr, qMin(pg, pb));
            int p_chroma = p_max - p_min;

            for (int i = 0; i < n_colors; i++)
            {
                QRgb ct = color_table[i];
                int cr = qRed(ct), cg = qGreen(ct), cb = qBlue(ct);
                int c_max = qMax(cr, qMax(cg, cb));
                int c_min = qMin(cr, qMin(cg, cb));
                int c_chroma = c_max - c_min;

                double d = RGB_color_Lab_difference_CIEDE2000(pr, pg, pb, cr, cg, cb);
                double luma_diff = qAbs((pr * 77 + pg * 150 + pb * 29) - (cr * 77 + cg * 150 + cb * 29)) / 256.0;
                d += luma_diff * 0.12;

                if (p_chroma > 18 && c_chroma == 0 && c_max > 0)
                {
                    d += p_chroma * 0.25;
                }

                if (d < best_diff)
                {
                    best_diff = d;
                    best_idx = i;
                }
            }
            converted.setPixel(ix, iy, best_idx);
        }
    }
    return converted;
}

QByteArray Zx4Processing::deflateRle8(QByteArray ba)
{
    QByteArray out;
    const char sep = static_cast<char>(0x80);
    for (int i = 0; i < ba.count();)
    {
        char current_char = ba[i];
        int count = 0;
        while (i < ba.count() && ba[i] == current_char)
        {
            ++count;
            ++i;
        }
        if (count == 1)
        {
            out += current_char;
            if (current_char == sep)
            {
                out += static_cast<char>(0x00);
            }
        }
        else if (count == 2 && current_char != sep)
        {
            out += current_char;
            out += current_char;
        }
        else
        {
            do
            {
                out += sep;
                out += static_cast<char>(qMin(255, count));
                out += current_char;
            }
            while ((count -= 255) > 0);
        }
    }
    return out;
}

QByteArray Zx4Processing::inflateRle8(QByteArray ba)
{
    QByteArray out;
    const char sep = static_cast<char>(0x80);
    for (int i = 0; i < ba.count(); ++i)
    {
        if (ba[i] != sep)
        {
            out += ba[i];
        }
        else
        {
            if (++i < ba.count())
            {
                int count = static_cast<quint8>(ba[i]);
                if (count == 0)
                {
                    out += sep;
                }
                else if (++i < ba.count())
                {
                    out += QByteArray(count, ba[i]);
                }
            }
        }
    }
    return out;
}

QCursor Zx4BaseObject::makeCursor(Zx4CursorType type)
{
    static QHash<int, QCursor> cache;
    if (cache.contains(type))
    {
        return cache.value(type);
    }

    QPixmap px(24, 24);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing, true);

    int hotX = 2;
    int hotY = 21;

    switch (type)
    {
    case CursorPencil:
    {
        hotX = 2;
        hotY = 21;
        // Остриё и корпус карандаша
        QPolygonF shaft;
        shaft << QPointF(5, 15) << QPointF(16, 4) << QPointF(19, 7) << QPointF(8, 18);
        QPolygonF cone;
        cone << QPointF(2, 21) << QPointF(5, 15) << QPointF(8, 18);
        QPolygonF cap;
        cap << QPointF(16, 4) << QPointF(18, 2) << QPointF(21, 5) << QPointF(19, 7);

        p.setPen(QPen(Qt::black, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolygon(shaft);
        p.drawPolygon(cone);
        p.drawPolygon(cap);

        p.setPen(QPen(Qt::white, 0.8));
        p.setBrush(QColor(245, 195, 45));
        p.drawPolygon(shaft);
        p.setBrush(QColor(240, 215, 175));
        p.drawPolygon(cone);
        p.setBrush(QColor(235, 110, 140));
        p.drawPolygon(cap);

        // Грифель
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);
        QPolygonF tip;
        tip << QPointF(2, 21) << QPointF(4, 17) << QPointF(6, 19);
        p.drawPolygon(tip);
        break;
    }
    case CursorBrush:
    {
        hotX = 2;
        hotY = 21;
        // Ручка кисти
        QPolygonF handle;
        handle << QPointF(9, 11) << QPointF(19, 2) << QPointF(21, 4) << QPointF(12, 14);
        // Ворс кисти
        QPainterPath bristles;
        bristles.moveTo(2, 21);
        bristles.quadTo(4, 15, 9, 11);
        bristles.lineTo(12, 14);
        bristles.quadTo(9, 20, 2, 21);

        p.setPen(QPen(Qt::black, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolygon(handle);
        p.drawPath(bristles);

        p.setPen(QPen(Qt::white, 0.8));
        p.setBrush(QColor(180, 115, 55));
        p.drawPolygon(handle);
        p.setBrush(QColor(55, 175, 250));
        p.drawPath(bristles);
        break;
    }
    case CursorFill:
    {
        hotX = 3;
        hotY = 20;
        // Наклонённое ведро
        QPolygonF bucket;
        bucket << QPointF(6, 11) << QPointF(14, 5) << QPointF(20, 12) << QPointF(12, 18);
        // Капля краски в точке заливки
        QPainterPath drop;
        drop.moveTo(6, 12);
        drop.quadTo(1, 17, 3, 20);
        drop.quadTo(6, 18, 6, 12);

        p.setPen(QPen(Qt::black, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolygon(bucket);
        p.drawPath(drop);

        p.setPen(QPen(Qt::white, 0.8));
        p.setBrush(QColor(210, 215, 225));
        p.drawPolygon(bucket);
        p.setBrush(QColor(45, 200, 95));
        p.drawPath(drop);

        // Дужка ведра
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::white, 1.5));
        p.drawArc(QRectF(11, 2, 9, 9), 20 * 16, 150 * 16);
        break;
    }
    case CursorEraser:
    {
        hotX = 4;
        hotY = 19;
        // Двухцветный ластик под углом
        QPolygonF pinkPart;
        pinkPart << QPointF(3, 16) << QPointF(10, 9) << QPointF(15, 14) << QPointF(8, 21);
        QPolygonF whitePart;
        whitePart << QPointF(10, 9) << QPointF(15, 4) << QPointF(20, 9) << QPointF(15, 14);

        p.setPen(QPen(Qt::black, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolygon(pinkPart);
        p.drawPolygon(whitePart);

        p.setPen(QPen(Qt::black, 0.9));
        p.setBrush(QColor(245, 130, 155));
        p.drawPolygon(pinkPart);
        p.setBrush(QColor(240, 245, 250));
        p.drawPolygon(whitePart);
        break;
    }
    case CursorMove:
    {
        hotX = 12;
        hotY = 12;
        QPainterPath cross;
        // Горизонтальная и вертикальная оси со стрелками
        cross.moveTo(12, 2);
        cross.lineTo(8, 6);
        cross.lineTo(10.5, 6);
        cross.lineTo(10.5, 10.5);
        cross.lineTo(6, 10.5);
        cross.lineTo(6, 8);
        cross.lineTo(2, 12);
        cross.lineTo(6, 16);
        cross.lineTo(6, 13.5);
        cross.lineTo(10.5, 13.5);
        cross.lineTo(10.5, 18);
        cross.lineTo(8, 18);
        cross.lineTo(12, 22);
        cross.lineTo(16, 18);
        cross.lineTo(13.5, 18);
        cross.lineTo(13.5, 13.5);
        cross.lineTo(18, 13.5);
        cross.lineTo(18, 16);
        cross.lineTo(22, 12);
        cross.lineTo(18, 8);
        cross.lineTo(18, 10.5);
        cross.lineTo(13.5, 10.5);
        cross.lineTo(13.5, 6);
        cross.lineTo(16, 6);
        cross.closeSubpath();

        p.setPen(QPen(Qt::black, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::white);
        p.drawPath(cross);
        break;
    }
    case CursorPipette:
    {
        hotX = 2;
        hotY = 21;
        // Стеклянная трубка пипетки
        QPolygonF tube;
        tube << QPointF(2, 21) << QPointF(5, 15) << QPointF(13, 7) << QPointF(16, 10) << QPointF(8, 18);
        // Поперечный упор
        QPolygonF collar;
        collar << QPointF(11, 5) << QPointF(18, 12) << QPointF(19.5, 10.5) << QPointF(12.5, 3.5);
        // Резиновая груша
        QPainterPath bulb;
        bulb.addEllipse(QPointF(18, 5), 3.8, 3.8);

        p.setPen(QPen(Qt::black, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolygon(tube);
        p.drawPolygon(collar);
        p.drawPath(bulb);

        p.setPen(QPen(Qt::white, 0.8));
        p.setBrush(QColor(175, 235, 255));
        p.drawPolygon(tube);
        p.setBrush(QColor(210, 210, 210));
        p.drawPolygon(collar);
        p.setBrush(QColor(235, 80, 95));
        p.drawPath(bulb);
        break;
    }
    case CursorStamp:
    {
        hotX = 12;
        hotY = 20;
        // Подошва штампа
        QRectF pad(4, 17, 16, 4);
        QRectF mount(6, 14, 12, 3);
        QPolygonF neck;
        neck << QPointF(9, 14) << QPointF(10.5, 9) << QPointF(13.5, 9) << QPointF(15, 14);

        p.setPen(QPen(Qt::black, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawRect(pad);
        p.drawRect(mount);
        p.drawPolygon(neck);
        p.drawEllipse(QPointF(12, 6), 4.5, 4.0);

        p.setPen(QPen(Qt::white, 0.8));
        p.setBrush(QColor(230, 95, 95));
        p.drawRect(pad);
        p.setBrush(QColor(190, 200, 210));
        p.drawRect(mount);
        p.setBrush(QColor(215, 150, 75));
        p.drawPolygon(neck);
        p.drawEllipse(QPointF(12, 6), 4.5, 4.0);
        break;
    }
    }

    QCursor cur(px, hotX, hotY);
    cache.insert(type, cur);
    return cur;
}

Zx4BaseObject::Zx4BaseObject(const Zx4Palette & _palettes) : palettes(_palettes)
{
    clearHistory();
}

Zx4BaseObject::~Zx4BaseObject()
{
    zx4_img.clear();
}

void Zx4BaseObject::setFileName(QString _f)
{
    file_name = _f;
}

QString Zx4BaseObject::fileName() const
{
    return file_name;
}

void Zx4BaseObject::setSourceImage(const QList<QImage> & i)
{
    src_imp_img = transformed_img = i;
    while (zx4_img.count() < i.count())
    {
        zx4_img << QSharedPointer<Zx4Image>(new Zx4Image());
    }
    while (zx4_img.count() > i.count())
    {
        zx4_img.removeLast();
    }
    current_frame = 0;
    for (int k = 0; k < i.count(); k++)
    {
        zx4_img[k]->resize(i[k].width(), i[k].height());
    }
    if (widget)
    {
        widget->update();
    }
}

void Zx4BaseObject::setSourceImage(const QImage & i)
{
    setSourceImage(QList<QImage>() << i);
}

void Zx4BaseObject::setSourceImage(QImageReader & i)
{
    QList<QImage> list;
    QImage img;
    if (i.jumpToImage(0))
    {
        while (!(img = i.read()).isNull())
        {
            list << img;
        }
    }
    setSourceImage(list);
}

const QImage & Zx4BaseObject::sourceImage(int frame) const
{
    static QImage placeholder_img;
    return frame >= 0 && frame < src_imp_img.count() ? src_imp_img[frame] : placeholder_img;
}

QImage & Zx4BaseObject::transformedImage(int frame)
{
    return transformed_img[frame];
}

void Zx4BaseObject::setImage(const QList<const Zx4Image *> & list)
{
    zx4_img.clear();
    current_frame = 0;
    for (int k = 0; k < list.count(); k++)
    {
        zx4_img << QSharedPointer<Zx4Image>(list[k]->clone());
    }
    if (widget)
    {
        widget->update();
    }
}

void Zx4BaseObject::setImage(const Zx4Image & i)
{
    setImage(QList<const Zx4Image *>() << &i);
}

const Zx4Image & Zx4BaseObject::image(int frame) const
{
    if (zx4_img.isEmpty())
    {
        zx4_img << QSharedPointer<Zx4Image>(new Zx4Image());
    }
    if (frame < 0 || frame >= zx4_img.count())
    {
        frame = 0;
    }
    return *zx4_img[frame];
}

void Zx4BaseObject::setCurrentColorIndex(int i)
{
    current_color_index = i;
    if (widget)
    {
        widget->update();
    }
}

int Zx4BaseObject::currentColorIndex() const
{
    return current_color_index;
}

void Zx4BaseObject::setImportPars(const Zx4ImportPars & i)
{
    imp_pars = i;
}

const Zx4ImportPars & Zx4BaseObject::importPars() const
{
    return imp_pars;
}

void Zx4BaseObject::setAutoPalette(bool auto_pal)
{
    auto_palette = auto_pal;
}

bool Zx4BaseObject::autoPalette() const
{
    return auto_palette;
}

void Zx4BaseObject::setPaletteSequenceIndex(int index, int max_index)
{
    palette_seq_max = qMax(1, max_index);
    palette_seq_index = std::clamp(index, 1, palette_seq_max);
}

int Zx4BaseObject::paletteSequenceIndex() const
{
    return palette_seq_index;
}

int Zx4BaseObject::paletteSequenceMax() const
{
    return palette_seq_max;
}

void Zx4BaseObject::setHorizontalMirror(bool v)
{
    horizontal_mirror = v;
}

bool Zx4BaseObject::horizontalMirror() const
{
    return horizontal_mirror;
}

void Zx4BaseObject::setXShift(double v)
{
    x_shift = v;
}

double Zx4BaseObject::xShift() const
{
    return x_shift;
}

void Zx4BaseObject::setYShift(double v)
{
    y_shift = v;
}

double Zx4BaseObject::yShift() const
{
    return y_shift;
}

void Zx4BaseObject::setXScale(double v)
{
    x_scale = v;
}

double Zx4BaseObject::xScale() const
{
    return x_scale;
}

void Zx4BaseObject::setYScale(double v)
{
    y_scale = v;
}

double Zx4BaseObject::yScale() const
{
    return y_scale;
}

void Zx4BaseObject::setRotate(double v)
{
    rotate = v;
}

double Zx4BaseObject::rotateAngle() const
{
    return rotate;
}

void Zx4BaseObject::setCanvas(QWidget * w)
{
    widget = w;
    widget->setMouseTracking(true);
    widget->installEventFilter(dynamic_cast<QWidget *>(this));
}

QWidget * Zx4BaseObject::canvas() const
{
    return widget;
}

void Zx4BaseObject::setCurrentFrame(int frame)
{
    if (frame < 0 || frame >= zx4_img.count())
    {
        frame = 0;
    }
    current_frame = frame;
}

int Zx4BaseObject::currentFrame() const
{
    return current_frame;
}

int Zx4BaseObject::framesCount() const
{
    return zx4_img.count();
}

void Zx4BaseObject::storeToHistory(QString desc)
{
    history.second = history.second.mid(0, history.first + 1);
    QList<QSharedPointer<Zx4Image>> list;
    for (const auto & entry : zx4_img)
    {
        list << QSharedPointer<Zx4Image>(entry->clone());
    }
    history.second << qMakePair(desc, list);
    history.first++;
}

void Zx4BaseObject::clearHistory()
{
    history = qMakePair(-1, QList<QPair<QString, QList<QSharedPointer<Zx4Image>>>>());
}

void Zx4BaseObject::undo()
{
    if (canUndo())
    {
        zx4_img.clear();
        for (const auto & entry : history.second.at(--history.first).second)
        {
            zx4_img << QSharedPointer<Zx4Image>(entry->clone());
        }
        if (!zx4_img.isEmpty() && !zx4_img.first()->isNull())
        {
            grid_x = zx4_img.first()->width();
            grid_y = zx4_img.first()->height();
            current_frame = std::clamp(current_frame, 0, int(zx4_img.count() - 1));
        }
        onHistoryRestored();
    }
}

void Zx4BaseObject::redo()
{
    if (canRedo())
    {
        zx4_img.clear();
        for (const auto & entry : history.second.at(++history.first).second)
        {
            zx4_img << QSharedPointer<Zx4Image>(entry->clone());
        }
        if (!zx4_img.isEmpty() && !zx4_img.first()->isNull())
        {
            grid_x = zx4_img.first()->width();
            grid_y = zx4_img.first()->height();
            current_frame = std::clamp(current_frame, 0, int(zx4_img.count() - 1));
        }
        onHistoryRestored();
    }
}

bool Zx4BaseObject::canUndo(QString * desc)
{
    const bool rc = history.first > 0 && history.first < history.second.count();
    if (rc && desc)
    {
        *desc = history.second.at(history.first).first;
    }
    return rc;
}

bool Zx4BaseObject::canRedo(QString * desc)
{
    const bool rc = history.first >= 0 && history.first < history.second.count() - 1;
    if (rc && desc)
    {
        *desc = history.second.at(history.first + 1).first;
    }
    return rc;
}

QString Zx4BaseObject::historyCurrentDescription() const
{
    if (history.first >= 0 && history.first < history.second.count())
    {
        return history.second.at(history.first).first;
    }
    return QString();
}
