#include <QSet>
#include <QApplication>
#include "zx4pixelizer.h"

void Zx4Palette::init()
{
#if 1
    color_table[ZxBlack] =          Qt::black;
    color_table[ZxBlue] =           Qt::darkBlue;
    color_table[ZxRed] =            Qt::darkRed;
    color_table[ZxMagenta] =        Qt::darkMagenta;
    color_table[ZxGreen] =          Qt::darkGreen;
    color_table[ZxCyan] =           Qt::darkCyan;
    color_table[ZxYellow] =         Qt::darkYellow;
    color_table[ZxWhite] =          Qt::lightGray;
    color_table[ZxBrightBlack] =    Qt::black;
    color_table[ZxBrightBlue] =     Qt::blue;
    color_table[ZxBrightRed] =      Qt::red;
    color_table[ZxBrightMagenta] =  Qt::magenta;
    color_table[ZxBrightGreen] =    Qt::green;
    color_table[ZxBrightCyan] =     Qt::cyan;
    color_table[ZxBrightYellow] =   Qt::yellow;
    color_table[ZxBrightWhite] =    Qt::white;
#else
    int level = 0xC0;
    int high_level = 0xFF;
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
#endif

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
            for (int c2i = 0; c2i < 8; c2i++)
            {
                for (int c3i = c2i; c3i < 8; c3i++)
                {
                    for (int b0 = 0; b0 < 2; b0++)
                    {
                        for (int b1 = 0; b1 < 2; b1++)
                        {
                            QColor c[4];
                            ZxColorEnum z[4];
                            c[0] = color_table[z[0] = static_cast<ZxColorEnum>(c0i + 8 * b0)];
                            c[1] = color_table[z[1] = static_cast<ZxColorEnum>(c1i + 8 * b0)];
                            c[2] = color_table[z[2] = static_cast<ZxColorEnum>(c2i + 8 * b1)];
                            c[3] = color_table[z[3] = static_cast<ZxColorEnum>(c3i + 8 * b1)];

                            QColor m[4];
                            m[0] = QColor::fromRgb((c[0].red() + c[2].red()) / 2, (c[0].green() + c[2].green()) / 2, (c[0].blue() + c[2].blue()) / 2);
                            m[1] = QColor::fromRgb((c[0].red() + c[3].red()) / 2, (c[0].green() + c[3].green()) / 2, (c[0].blue() + c[3].blue()) / 2);
                            m[2] = QColor::fromRgb((c[1].red() + c[2].red()) / 2, (c[1].green() + c[2].green()) / 2, (c[1].blue() + c[2].blue()) / 2);
                            m[3] = QColor::fromRgb((c[1].red() + c[3].red()) / 2, (c[1].green() + c[3].green()) / 2, (c[1].blue() + c[3].blue()) / 2);

                            QList<QRgb> ml1;
                            ml1 << m[0].rgb() << m[1].rgb() << m[2].rgb() << m[3].rgb();
                            if (ml1.toSet().count() < 4) // if (ml1.toSet().count() != 4)
                            {
                                continue;
                            }
                            QList<QRgb> ml2 = ml1;
                            qSort(ml1);
                            if (ml1 != ml2)
                            {
                                continue;
                            }

                            QList<QColor> list;
                            list << m[0] << m[1] << m[2] << m[3];
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
        qSort(i.value() = i.value().toSet().toList());
    }
    for (auto i = palettes_by_color.begin(); i != palettes_by_color.end(); ++i)
    {
        qApp->processEvents();
        for (auto k = i + 1; k != palettes_by_color.end(); ++k)
        {
            auto list = i.value().toSet().intersect(k.value().toSet()).toList();
            if (!list.isEmpty())
            {
                qSort(palettes_by_2colors[QPair<QColor, QColor>(i.key(), k.key())] = list);
            }
        }
    }
    for (auto i = palettes_by_2colors.begin(); i != palettes_by_2colors.end(); ++i)
    {
        qApp->processEvents();
        for (auto k = palettes_by_color.begin(); k != palettes_by_color.end(); ++k)
        {
            if (k.key().rgba() <= i.key().second.rgba())
            {
                continue;
            }
            auto list = i.value().toSet().intersect(k.value().toSet()).toList();
            if (!list.isEmpty())
            {
                qSort(palettes_by_3colors[i.key()][k.key()] = list);
            }
        }
    }
    for (auto i = palettes_by_2colors.begin(); i != palettes_by_2colors.end(); ++i)
    {
        qApp->processEvents();
        for (auto k = i + 1; k != palettes_by_2colors.end(); ++k)
        {
            if (k.key().first.rgba() <= i.key().second.rgba())
            {
                continue;
            }
            auto list = i.value().toSet().intersect(k.value().toSet()).toList();
            if (!list.isEmpty())
            {
                qSort(palettes_by_4colors[i.key()][k.key()] = list);
            }
        }
    }
}

QMultiMap<double, int> Zx4Palette::selectPalette(QMap<QColor, int> color_distrib4) const
{
    QMultiMap<double, int> dist;
    for (int i = 0; i < palettes.count(); i++)
    {
        QList<QColor> pal = palettes[i];
        if (pal[0] == pal[1] || pal[2] == pal[3] || pal[0] == pal[2] || pal[1] == pal[3] || pal[1] == pal[2])
        {
            continue;
        }
        double d = 0;
        foreach (QColor p, pal)
        {
            QColor p_hsl = p.toHsl();
            int p_h = p_hsl.hslHue();
            int p_s = p_hsl.hslSaturation();
            int p_l = p_hsl.lightness();
            for(QMap<QColor, int>::iterator c = color_distrib4.begin(); c != color_distrib4.end(); ++c)
            {
                QColor c_hsl = c.key().toHsl();
                int c_h = c_hsl.hslHue();
                int c_s = c_hsl.hslSaturation();
                int c_l = c_hsl.lightness();

                double _d = 0;
                if (p_h != -1 && c_h != -1)
                {
                    if (p_h < c_h)
                    {
                        p_h += 360;
                    }
                    _d = p_h - c_h;
                    if (_d > 180)
                    {
                        _d = 360 - _d;
                    }
                }
                else
                if (p_h != c_h)
                {
                    _d += 90;
                }

//                _d += 1.0 * (qAbs(p_s - c_s) + 0.1 * qAbs(p_l - c_l));
                _d += 1.0 * (qAbs(p_s - c_s) + 0.1 * qAbs(p_l - c_l));
//
                /*
                if (qAbs(p_s - c_s) + qAbs(p_l - c_l) > 96)
                {
                    _d += 90;
                }
                */

                d += _d / c.value();
            }
        }
        if (d > 0)
        {
            dist.insert(d, i);
        }
    }

//    qDebug() << dist;

    return dist;
}

QMultiMap<double, int> Zx4Palette::selectPalette(QImage img) const
{
    if (img.colorCount() >= 4)
    {
        QVector<QRgb> colors = img.colorTable();
        if (palettes_by_4colors.contains(QPair<QColor, QColor>(colors[0], colors[1])) && palettes_by_4colors[QPair<QColor, QColor>(colors[0], colors[1])].contains(QPair<QColor, QColor>(colors[2], colors[3])))
        {
            QList<int> list = palettes_by_4colors[QPair<QColor, QColor>(colors[0], colors[1])][QPair<QColor, QColor>(colors[2], colors[3])];
            if (!list.isEmpty())
            {
                int p_idx = list.first();
                QMultiMap<double, int> map;
                map.insert(0, p_idx);
                return map;
            }
        }
    }
    return selectPalette(Zx4Processing::colorDistribution4(img));
}


QImage Zx4Processing::scaleImage(QImage img, int w, int h)
{
    img = img.convertToFormat(QImage::Format_ARGB32);
    QImage out(img.scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation).size(), QImage::Format_ARGB32);
    out.fill(QColor(0, 0, 0, 0));
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
    bool src_alpha_channel = img.hasAlphaChannel() && qAlpha(img.pixel(0, 0)) < 128;

    QMap<QColor, int> color_distrib;
    QMultiMap<int, QColor> color_distrib2;
    QImage detect_colors_img = img.convertToFormat(QImage::Format_Indexed8);
    QByteArray ba = QByteArray::fromRawData(reinterpret_cast<const char *>(detect_colors_img.constBits()), detect_colors_img.width() * detect_colors_img.height());
    QVector<QRgb> color_table = detect_colors_img.colorTable();
    for (int i = 0; i < color_table.count(); i++)
    {
        int n = ba.count(i);
        if (n > 0)
        {
            color_distrib[color_table[i]] += n;
        }
    }

    if (use_alpha)
    {
        for (int y = 0; y < img.height(); y++)
        {
            for (int x = 0; x < img.width(); x++)
            {
                if ((src_alpha_channel && qAlpha(img.pixel(x, y)) < 128) || (!src_alpha_channel && detect_colors_img.pixel(0, 0) == detect_colors_img.pixel(x, y)))
                {
                    color_distrib.remove(detect_colors_img.pixel(x, y));
                }
            }
        }
    }

    for (auto i = color_distrib.begin(); i != color_distrib.end(); ++i)
    {
        if (i.value() > 0)
        {
            color_distrib2.insert(i.value(), i.key());
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
        if (c1_h < c2_h)
        {
            c1_h += 360;
        }
        int _d = c1_h - c2_h;
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
    QColor prev_c, val_c;
    for (auto i = color_distrib2.end(); i != color_distrib2.begin() && color_distrib4.count() < 4;)
    {
        --i;
        QColor c = i.value().toHsl();
        if (!prev_c.isValid() || !alikeColors(c, prev_c))
        {
            color_distrib4[val_c = c] = i.key();
        }
        else
        if (val_c.isValid())
        {
            color_distrib4[val_c] += i.key();
        }
        prev_c = c;
    }

    return color_distrib4;
}

QImage Zx4Processing::importImage(QImage img, QVector<QRgb> color_table, bool use_alpha)
{
    QRgb alpha_pixel;
    bool src_alpha_channel = img.hasAlphaChannel() && qAlpha(img.pixel(0, 0)) < 128;
    if (use_alpha)
    {
        if (!src_alpha_channel)
        {
            alpha_pixel = img.pixel(0, 0);
        }
        else
        {
            alpha_pixel = color_table.first() & 0x00FFFFFF; // duplicate color with full opacity
            for (int y = 0; y < img.height(); y++)
            {
                for (int x = 0; x < img.width(); x++)
                {
                    if (qAlpha(img.pixel(x, y)) < 128)
                    {
                        img.setPixel(x, y, alpha_pixel);
                    }
                }
            }
        }

        color_table << alpha_pixel;
    }

    img = img.convertToFormat(QImage::Format_Indexed8, color_table);

    if (use_alpha)
    {
        if (!src_alpha_channel)
        {
            alpha_pixel = img.pixel(0, 0);
            for (int y = 0; y < img.height(); y++)
            {
                for (int x = 0; x < img.width(); x++)
                {
                    if (img.pixel(x, y) == alpha_pixel)
                    {
                        img.setPixel(x, y, 4); // alpha_pixel index
                    }
                }
            }
        }
        color_table.last() = qRgba(0, 0, 0, 0);
        img.setColorTable(color_table);
    }

    return img;
}

QByteArray Zx4Processing::deflateRle8(QByteArray ba)
{
    QByteArray out;
    const char sep = (char)0x80;
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
                out += (char)0x00;
            }
        }
        else
        if (count == 2 && current_char != sep)
        {
            out += current_char;
            out += current_char;
        }
        else
        {
            do
            {
                out += sep;
                out += (char)qMin(255, count);
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
    const char sep = (char)0x80;
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
                int count = ba[i];
                if (count == 0)
                {
                    out += sep;
                }
                else
                {
                    if (++i < ba.count())
                    {
                        out += QByteArray(count, ba[i]);
                    }
                }
            }
        }
    }
    return out;
}
