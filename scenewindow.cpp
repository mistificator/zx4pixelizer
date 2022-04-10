#include <QFileInfo>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include "color_conversions.h"
#include "scenewindow.h"
#include "ui_scenewindow.h"

SceneWindow::SceneWindow(const Zx4Palette & palettes, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SceneWindow),
    palettes(palettes)
{
    ui->setupUi(this);
    ui->canvas->installEventFilter(this);

    setFileName(QString());
}

SceneWindow::~SceneWindow()
{
    delete ui;
}

void SceneWindow::setFileName(QString s)
{
    file_name = s;
    setWindowTitle((!file_name.isEmpty() ? QFileInfo(file_name).fileName() : "untitled") + " - Scene");
}

QString SceneWindow::fileName() const
{
    return file_name;
}

void SceneWindow::setSourceImage(const QImage & i)
{
    scene.resize(i.width(), i.height());
    src_imp_img = i;
    ui->canvas->update();
}

const QImage & SceneWindow::sourceImage() const
{
    return src_imp_img;
}

const QImage & SceneWindow::renderedImage() const
{
    return rdr_img;
}

QSize SceneWindow::sizeHint() const
{
    return QSize(640, 640);
}

QLabel * SceneWindow::canvas() const
{
    return ui->canvas;
}

void SceneWindow::setImage(const Zx4Image & i)
{
    scene = i;
    ui->canvas->update();
}

const Zx4Image & SceneWindow::image() const
{
    return scene;
}

void SceneWindow::setAttr(QMap<int, QMap<int, int>> a)
{
    scene_attr = a;
    ui->canvas->update();
}

QMap<int, QMap<int, int>> SceneWindow::attr() const
{
    return scene_attr;
}


QPoint SceneWindow::currentChar() const
{
    return selected_char;
}

void SceneWindow::setAttr(QPoint p, int p_idx)
{
    scene_attr[p.x()][p.y()] = p_idx;
}

void SceneWindow::setAttr(int p_idx)
{
    setAttr(selected_char, p_idx);
}

void SceneWindow::setCurrentColorIndex(int i)
{
    current_color_index = i;
}

int SceneWindow::currentColorIndex() const
{
    return current_color_index;
}

void SceneWindow::setPixel(QPoint p, int color_index)
{
    if (p.y() < 0 || p.y() >= scene.color_layer0.count())
    {
        return;
    }
    if (p.x() < 0 || p.x() >= scene.color_layer0[p.y()].count())
    {
        return;
    }
    scene.color_layer0[p.y()][p.x()] = color_index == 0 || color_index == 1 ? 0 : 1;
    scene.color_layer1[p.y()][p.x()] = color_index == 0 || color_index == 2 ? 0 : 1;
}

bool SceneWindow::eventFilter(QObject * o, QEvent * ev)
{
    if (o == ui->canvas)
    {
        switch (ev->type())
        {
            case QEvent::Paint:
                drawGrid();
            break;
            case QEvent::MouseButtonPress:
            case QEvent::MouseMove:
            case QEvent::MouseButtonDblClick:
            {
                QMouseEvent * me = dynamic_cast<QMouseEvent *>(ev);
                double step = qMin(1.0 * grid_x / src_imp_img.width(), 1.0 * grid_y / src_imp_img.height());

                QPoint pt = me->pos();
                int x = (pt.x() - 1) / step;
                int y = (pt.y() - 1) / step;

                if (me->buttons().testFlag(Qt::LeftButton))
                {
                    if (me->modifiers().testFlag(Qt::ShiftModifier))
                    {
                        if (!selected_char.isNull())
                        {
                            update_char = QPoint(x / 8, y / 8);
                            importChar(
                                update_char,
                                scene_attr[update_char.x()][update_char.y()] = scene_attr[selected_char.x()][selected_char.y()],
                                src_imp_img.convertToFormat(QImage::Format_Indexed8).copy(update_char.x() * 8, update_char.y() * 8, 8, 8));
                        }
                    }
                    else
                    if (me->modifiers().testFlag(Qt::ControlModifier))
                    {
                        if (me->type() == QMouseEvent::MouseButtonDblClick && selected_char == QPoint(x / 8, y / 8))
                        {
                            selected_char = QPoint();
                        }
                        else
                        {
                            selected_char = QPoint(x / 8, y / 8);
                            emit charSelected(scene_attr[selected_char.x()][selected_char.y()]);
                        }
                    }
                    else
                    if (me->modifiers().testFlag(Qt::NoModifier))
                    {
                        if (selected_char == QPoint(x / 8, y / 8))
                        {
                            setPixel(QPoint(x, y), currentColorIndex());
                        }
                    }
                    QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);
                }
            }
            break;
            case QEvent::MouseButtonRelease:
            {
                QMouseEvent * me = dynamic_cast<QMouseEvent *>(ev);
                update_char = QPoint();
                QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);
            }
            break;
            default:
            break;
        }
    }
    return QWidget::eventFilter(o, ev);
}

void SceneWindow::drawGrid()
{
    scene.resize(src_imp_img.width(), src_imp_img.height());
    rdr_img = QImage(src_imp_img.width(), src_imp_img.height(), QImage::Format_RGB32);
    for (int y = 0; y < rdr_img.height(); y++)
    {
        for (int x = 0; x < rdr_img.width(); x++)
        {
            int p_idx = scene_attr[x / 8][y / 8];
            auto c = palettes.palettes[p_idx];

            char l0 = scene.color_layer0[y][x];
            char l1 = scene.color_layer1[y][x];
            rdr_img.setPixelColor(x, y, !l0 && !l1 ? c[0] : l0 && !l1 ? c[2] : !l0 && l1 ? c[1] : c[3]);
        }
    }
    QImage out = rdr_img.scaled(ui->canvas->size(), Qt::KeepAspectRatio);
    grid_x = out.width();
    grid_y = out.height();

    QPainter p(ui->canvas);
    p.fillRect(ui->canvas->rect(), Qt::black);
    p.drawImage(0, 0, out);

    double kx = 8.0 * grid_x / src_imp_img.width();
    double ky = 8.0 * grid_y / src_imp_img.height();
    auto list = QList<QPair<QPoint, QColor> >() << QPair<QPoint, QColor>(selected_char, QColor(0, 255, 0, 128)) << QPair<QPoint, QColor>(update_char, QColor(255, 0, 0, 128));
    for (auto i = list.begin(); i != list.end(); ++i)
    {
        if (!i->first.isNull())
        {
            p.setPen(QPen(i->second, 3, Qt::DotLine, Qt::RoundCap));
            p.drawRect(i->first.x() * kx, i->first.y() * ky, kx, ky);
        }
    }

    QMetaObject::invokeMethod(this, "previewUpdateIsNeeded", Qt::QueuedConnection);
}

int SceneWindow::selectPalette(QImage small_quad, Zx4Palette::SelectPaletteMethod method) const
{
    auto small_dist = Zx4Processing::colorDistribution(small_quad, false);

    int p_idx = -1;
    if (small_dist.isEmpty())
    {
        return p_idx;
    }

    int k = 0;
    QMap<int, QColor> dist4;
    for (auto i = --small_dist.end(); k < 4; --i, ++k)
    {
        int key = i.key();
        while (dist4.contains(key))
        {
            key++;
        }
        dist4[key] = i.value();
        if (i == small_dist.begin())
        {
            break;
        }
    }

    switch (method)
    {
    case Zx4Palette::SelectPaletteCIE76:
    case Zx4Palette::SelectPaletteCIE94:
    case Zx4Palette::SelectPaletteCIEDE2000:
    {
        QMap<double, int> sorted_p_idx;
        int k = 0;
        foreach (QList<QColor> colors, palettes.palettes)
        {
            double d = 0;
            foreach (QColor color, colors)
            {
                for (auto i = dist4.begin(); i != dist4.end(); ++i)
                {
                    const float m = i.key(); //::sqrtf(i.key() + 1);
                    switch (method)
                    {
                    case Zx4Palette::SelectPaletteCIE76:
                        d += RGB_color_Lab_difference_CIE76(color.red(), color.green(), color.blue(), i->red(), i->green(), i->blue()) * m;
                        break;
                    case Zx4Palette::SelectPaletteCIE94:
                        d += RGB_color_Lab_difference_CIE94(color.red(), color.green(), color.blue(), i->red(), i->green(), i->blue()) * m;
                        break;
                    case Zx4Palette::SelectPaletteCIEDE2000:
                        d += RGB_color_Lab_difference_CIEDE2000(color.red(), color.green(), color.blue(), i->red(), i->green(), i->blue()) * m;
                        break;
                    default:
                        break;
                    }
                }
            }
            sorted_p_idx[d] = k++;
        }
        if (!sorted_p_idx.isEmpty())
        {
            p_idx = * sorted_p_idx.begin();
        }
        break;
    }
    default:
        do
        {
            QList<QColor> colors = dist4.values();
            qSort(colors); // indexes "palettes_by_color" sorted by color

            const double ls = 0.85;
            const double t = 0.1;

            int hue_precision = 10;
            do
            {
                int light_satur_precision = 16;
                do
                {
                    if (colors.count() == 1)
                    {
                        for (auto i = palettes.palettes_by_color.begin(); i != palettes.palettes_by_color.end(); ++i)
                        {
                            if (Zx4Processing::alikeColors(i.key(), colors.first(), hue_precision, light_satur_precision, ls * light_satur_precision))
                            {
                                p_idx = i.value().at(i.value().count() * t);
                                break;
                            }
                        }
                    }
                    else
                    if (colors.count() == 2)
                    {
                        for (auto i = palettes.palettes_by_2colors.begin(); i != palettes.palettes_by_2colors.end(); ++i)
                        {
                            if (Zx4Processing::alikeColors(i.key().first, colors.first(), hue_precision, light_satur_precision, ls * light_satur_precision) && Zx4Processing::alikeColors(i.key().second, colors.last(), hue_precision, light_satur_precision, ls * light_satur_precision))
                            {
                                int _p_idx = i.value().at(i.value().count() * t);
                                if (checkPalette(small_quad, _p_idx))
                                {
                                    p_idx = _p_idx;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    if (colors.count() == 3)
                    {
                        for (auto i = palettes.palettes_by_3colors.begin(); i != palettes.palettes_by_3colors.end() && p_idx == -1; ++i)
                        {
                            if (Zx4Processing::alikeColors(i.key().first, colors.at(0), hue_precision, light_satur_precision, ls * light_satur_precision) && Zx4Processing::alikeColors(i.key().second, colors.at(1), hue_precision, light_satur_precision, ls * light_satur_precision))
                            {
                                for (auto k = i.value().begin(); k != i.value().end(); ++k)
                                {
                                    if (Zx4Processing::alikeColors(k.key(), colors.at(2)))
                                    {
                                        int _p_idx = k.value().at(k.value().count() * t);
                                        if (checkPalette(small_quad, _p_idx))
                                        {
                                            p_idx = _p_idx;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    if (colors.count() == 4)
                    {
                        for (auto i = palettes.palettes_by_4colors.begin(); i != palettes.palettes_by_4colors.end() && p_idx == -1; ++i)
                        {
                            if (Zx4Processing::alikeColors(i.key().first, colors.at(0), hue_precision, light_satur_precision, ls * light_satur_precision) && Zx4Processing::alikeColors(i.key().second, colors.at(1), hue_precision, light_satur_precision, ls * light_satur_precision))
                            {
                                for (auto k = i.value().begin(); k != i.value().end(); ++k)
                                {
                                    if (Zx4Processing::alikeColors(k.key().first, colors.at(2), hue_precision) && Zx4Processing::alikeColors(k.key().second, colors.at(3), hue_precision))
                                    {
                                        int _p_idx = k.value().at(k.value().count() * t);
                                        if (checkPalette(small_quad, _p_idx))
                                        {
                                            p_idx = _p_idx;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    light_satur_precision += 16;
                }
                while (p_idx == -1 && light_satur_precision <= 64);
                hue_precision += 5;
            }
            while (p_idx == -1 && hue_precision < 100 / (colors.count()));

            dist4.erase(dist4.begin());
        }
        while (p_idx == -1 && !dist4.isEmpty());

        break;
    }

    return p_idx;
}

bool SceneWindow::checkPalette(QImage small_quad, int p_idx) const
{
    if (p_idx == -1)
    {
        return false;
    }
    QVector<QRgb> v;
    foreach (QColor c, palettes.palettes[p_idx])
    {
        v << c.rgb();
    }
    auto check_dist = Zx4Processing::colorDistribution(small_quad.convertToFormat(QImage::Format_RGB32).convertToFormat(QImage::Format_Indexed8, v).convertToFormat(QImage::Format_RGB32), false);
    return check_dist.count() > 1;
}

void SceneWindow::importChar(QPoint char_pos, int p_idx, QImage small_quad)
{
    if (p_idx < 0 || scene.isNull())
    {
        return;
    }

    QVector<QRgb> color_table;
    foreach(QColor c, palettes.palettes[p_idx])
    {
        color_table << c.rgb();
    }
    small_quad = small_quad.convertToFormat(QImage::Format_RGB32).convertToFormat(QImage::Format_Indexed8, color_table);

    scene_attr[char_pos.x()][char_pos.y()] = p_idx;
    for (int iy = 0; iy < 8; iy++)
    {
        int y = iy + char_pos.y() * 8;
        if (y < src_imp_img.height())
        for (int ix = 0; ix < 8; ix++)
        {
            int x = ix + char_pos.x() * 8;
            if (x < src_imp_img.width())
            {
                int idx = small_quad.pixelIndex(ix, iy);
                if (idx >= 0 && idx < 4)
                {
                    scene.color_layer0[y][x] = idx == 0 || idx == 1 ? 0 : 1;
                    scene.color_layer1[y][x] = idx == 0 || idx == 2 ? 0 : 1;
                    scene.alpha_layer[y][x] = 1;
                }
                else
                {
                    qDebug() << "wrong color index" << idx;
                }
            }
        }
    }
}

#define GetY(y)         (((y & 0xc0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2))
#define GetXY(x, y)     (GetY(y) | (x & 0x1F))

QPair<QByteArray, QByteArray> SceneWindow::makeScreens() const
{
    QByteArray scr0(32 * 192 + 32 * 24, 0);
    QByteArray scr1 = scr0;
    for (int y = 0; y < scene.color_layer0.count(); y++)
    {
        const QVector<char> & line0 = scene.color_layer0[y];
        const QVector<char> & line1 = scene.color_layer1[y];
        for (int x = 0; x < line0.count() / 8; x++)
        {
            int xy = GetXY(x, y);
            for (int bit = 0; bit < 8; bit++)
            {
                scr0[xy] = static_cast<char>(scr0[xy] | (static_cast<int>(line0[(8 * x) + (7 - bit)]) << bit));
                scr1[xy] = static_cast<char>(scr1[xy] | (static_cast<int>(line1[(8 * x) + (7 - bit)]) << bit));
            }
        }
    }
    int attr_offs = 192 * 32;
    for (int y = 0; y < (scene.color_layer0.count() + 7) / 8; y++)
    {
        for (int x = 0; x < scene.color_layer0[y].count() / 8; x++)
        {
            int p_idx = scene_attr[x][y];
            QList<ZxColorEnum> z = palettes.color_palette_table[p_idx];
            scr0[attr_offs + y * 32 + x] = (z[1] & 0x07) | ((z[0] & 0x0F) << 3);
            scr1[attr_offs + y * 32 + x] = (z[3] & 0x07) | ((z[2] & 0x0F) << 3);
        }
    }
    return QPair<QByteArray, QByteArray>(scr0, scr1);
}
