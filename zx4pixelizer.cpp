#include <QSet>
#include <QApplication>
#include "zx4pixelizer.h"

void Zx4Palette::init()
{
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
        for (int c1i = 0; c1i < 8; c1i++)
        {
            for (int c2i = 0; c2i < 8; c2i++)
            {
                for (int c3i = 0; c3i < 8; c3i++)
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
                            if (ml1.toSet().count() != 4)
                            {
                                continue;
                            }
                            QList<QRgb> ml2 = ml1;
                            qSort(ml1);
                            if (ml1 != ml2)
                            {
                                continue;
                            }

                            palettes << (QList<QColor>() << m[0] << m[1] << m[2] << m[3]);

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
