#include <QFileInfo>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include "color_conversions.h"
#include "scenewindow.h"
#include "ui_scenewindow.h"

SceneWindow::SceneWindow(const Zx4Palette & palettes, QWidget *parent) :
    QWidget(parent), Zx4BaseObject(palettes), ui(new Ui::SceneWindow), scene(* new Zx4ImageWithAttributes())
{
    ui->setupUi(this);
    setCanvas(ui->canvas);

    setFileName(QString());
    selected_char = update_char = QPoint(-1, -1);
}

SceneWindow::~SceneWindow()
{
    delete & scene;
    delete ui;
}

void SceneWindow::setFileName(QString s)
{
    Zx4BaseObject::setFileName(s);
    setWindowTitle((!file_name.isEmpty() ? QFileInfo(file_name).fileName() : "untitled") + " - Scene");
}

const QImage & SceneWindow::renderedImage() const
{
    return rdr_img;
}

QSize SceneWindow::sizeHint() const
{
    return QSize(640, 640);
}

void SceneWindow::setImage(const Zx4Image & i)
{
    if (!dynamic_cast<const Zx4ImageWithAttributes *>(& i))
    {
        return;
    }
    if (zx4_img)
    {
        delete zx4_img;
    }
    scene = * dynamic_cast<const Zx4ImageWithAttributes *>(zx4_img = i.clone());
    if (widget)
    {
        widget->update();
    }
}

void SceneWindow::setAttr(Zx4ImageWithAttributes::Attr a)
{
    scene.attr = a;
    ui->canvas->update();
}

Zx4ImageWithAttributes::Attr SceneWindow::attr() const
{
    return scene.attr;
}

QPoint SceneWindow::currentChar() const
{
    return selected_char;
}

void SceneWindow::setAttr(QPoint p, int p_idx)
{
    scene.attr[p.x()][p.y()] = p_idx;
}

void SceneWindow::setAttr(int p_idx)
{
    setAttr(selected_char, p_idx);
}

void SceneWindow::setPixel(QPoint p, int color_index)
{
    if (p.y() < 0 || p.y() >= scene.height())
    {
        return;
    }
    if (p.x() < 0 || p.x() >= scene.width())
    {
        return;
    }
    scene.color_layer0[p.x()][p.y()] = color_index == 0 || color_index == 1 ? Zx4Image::BitOff : Zx4Image::BitOn;
    scene.color_layer1[p.x()][p.y()] = color_index == 0 || color_index == 2 ? Zx4Image::BitOff : Zx4Image::BitOn;
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
                double step = qMin(1.0 * grid_x / transformed_img.width(), 1.0 * grid_y / transformed_img.height());

                QPoint pt = me->pos();
                int x = qMin<int>(qMax<int>(pt.x() / step, 0), transformed_img.width() - 1);
                int y = qMin<int>(qMax<int>(pt.y() / step, 0), transformed_img.height() - 1);

                if (me->buttons().testFlag(Qt::LeftButton))
                {
                    if (me->modifiers().testFlag(Qt::ShiftModifier))
                    {
                        if (selected_char != QPoint(-1, -1))
                        {
                            update_char = QPoint(x / 8, y / 8);
                            importChar(
                                update_char,
                                scene.attr[update_char.x()][update_char.y()] = scene.attr[selected_char.x()][selected_char.y()],
                                transformed_img.convertToFormat(QImage::Format_Indexed8).copy(update_char.x() * 8, update_char.y() * 8, 8, 8));
                        }
                    }
                    else
                    if (me->modifiers().testFlag(Qt::ControlModifier))
                    {
                        if (me->type() == QMouseEvent::MouseButtonDblClick && selected_char == QPoint(x / 8, y / 8))
                        {
                            selected_char = QPoint(-1, -1);
                        }
                        else
                        {
                            selected_char = QPoint(x / 8, y / 8);
                            emit charSelected(scene.attr[selected_char.x()][selected_char.y()]);
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
                update_char = QPoint(-1, -1);
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
    scene.resize(transformed_img.width(), transformed_img.height());
    rdr_img = QImage(transformed_img.width(), transformed_img.height(), QImage::Format_RGB32);
    for (int y = 0; y < rdr_img.height(); y++)
    {
        for (int x = 0; x < rdr_img.width(); x++)
        {
            int p_idx = scene.attr[x / 8][y / 8];
            auto c = palettes.palettes[p_idx];

            char l0 = scene.color_layer0[x][y];
            char l1 = scene.color_layer1[x][y];
            rdr_img.setPixelColor(x, y, !l0 && !l1 ? c[0] : l0 && !l1 ? c[2] : !l0 && l1 ? c[1] : c[3]);
        }
    }
    QImage out = rdr_img.scaled(ui->canvas->size(), Qt::KeepAspectRatio);
    grid_x = out.width();
    grid_y = out.height();

    QPainter p(ui->canvas);
    p.fillRect(ui->canvas->rect(), Qt::black);
    p.drawImage(0, 0, out);

    double kx = 8.0 * grid_x / transformed_img.width();
    double ky = 8.0 * grid_y / transformed_img.height();
    const auto list = QList<QPair<QPoint, QColor> >() << QPair<QPoint, QColor>(selected_char, QColor(0, 255, 0, 128)) << QPair<QPoint, QColor>(update_char, QColor(255, 0, 0, 128));
    const int pen_width = 3;
    for (auto i = list.begin(); i != list.end(); ++i)
    {
        if (i->first != QPoint(-1, -1))
        {
            p.setPen(QPen(i->second, pen_width, Qt::DotLine, Qt::RoundCap));
            p.drawRect(i->first.x() * kx - pen_width / 2.0, i->first.y() * ky - pen_width / 2.0, kx + pen_width, ky + pen_width);
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
        do
        {
            int k = 0;
            foreach (QList<QColor> colors, palettes.palettes)
            {
                double d = 0;
                auto _dist4 = dist4;
                foreach (QColor color, colors)
                {
                    QMap<double, int> d_map;
                    for (auto i = _dist4.begin(); i != _dist4.end(); ++i)
                    {
                        switch (method)
                        {
                        case Zx4Palette::SelectPaletteCIE76:
                            d_map[RGB_color_Lab_difference_CIE76(color.red(), color.green(), color.blue(), i->red(), i->green(), i->blue())] = i.key();
                            break;
                        case Zx4Palette::SelectPaletteCIE94:
                            d_map[RGB_color_Lab_difference_CIE94(color.red(), color.green(), color.blue(), i->red(), i->green(), i->blue())] = i.key();
                            break;
                        case Zx4Palette::SelectPaletteCIEDE2000:
                            d_map[RGB_color_Lab_difference_CIEDE2000(color.red(), color.green(), color.blue(), i->red(), i->green(), i->blue())] = i.key();
                            break;
                        default:
                            break;
                        }
                    }
                    _dist4.remove(d_map.begin().value());
                    d += d_map.begin().key() * d_map.begin().value();
                    if (_dist4.isEmpty())
                    {
                        break;
                    }
                }
                d /= dist4.count();
                while (sorted_p_idx.contains(d))
                {
                    d += 1e-6;
                }
                sorted_p_idx[d] = k++;
            }
            dist4.erase(dist4.begin());
        }
        while (dist4.count() > 1);
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

    small_quad = Zx4Processing::importImage(small_quad.convertToFormat(QImage::Format_RGB32), palettes.colorTable(p_idx));

    #pragma omp critical
        scene.attr[char_pos.x()][char_pos.y()] = p_idx;

    for (int iy = 0; iy < 8; iy++)
    {
        int y = iy + char_pos.y() * 8;
        if (y < transformed_img.height())
        for (int ix = 0; ix < 8; ix++)
        {
            int x = ix + char_pos.x() * 8;
            if (x < transformed_img.width())
            {
                int idx = small_quad.pixelIndex(ix, iy);
                if (idx >= 0 && idx < 4)
                #pragma omp critical
                {
                    scene.color_layer0[x][y] = idx == 0 || idx == 1 ? Zx4Image::BitOff : Zx4Image::BitOn;
                    scene.color_layer1[x][y] = idx == 0 || idx == 2 ? Zx4Image::BitOff : Zx4Image::BitOn;
                    scene.alpha_layer[x][y] = Zx4Image::BitOn;
                }
                else
                {
                    qDebug() << "wrong color index" << idx;
                }
            }
        }
    }
}

void SceneWindow::invertColors()
{
    if (selected_char == QPoint(-1, -1))
    {
        return;
    }
    for (int y = selected_char.y() * 8; y < qMin((selected_char.y() + 1) * 8, scene.height()); y++)
    {
        for (int x = selected_char.x() * 8; x < qMin((selected_char.x() + 1) * 8, scene.width()); x++)
        {
            scene.color_layer0[x][y] = scene.color_layer0[x][y] ? Zx4Image::BitOff : Zx4Image::BitOn;
            scene.color_layer1[x][y] = scene.color_layer1[x][y] ? Zx4Image::BitOff : Zx4Image::BitOn;
        }
    }
    ui->canvas->update();
}
