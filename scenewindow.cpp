#include <QFileInfo>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QGuiApplication>
#include <algorithm>
#include "color_conversions.h"
#include "scenewindow.h"
#include "ui_scenewindow.h"

SceneWindow::SceneWindow(const Zx4Palette & palettes, QWidget *parent) :
    QWidget(parent), Zx4BaseObject(palettes), ui(new Ui::SceneWindow)
{
    zx4_img << QSharedPointer<Zx4Image>(new Zx4ImageWithAttributes());
    ui->setupUi(this);
    setCanvas(ui->canvas);
    SceneWindow::setFileName(QString());
    selected_char = update_char = QPoint(-1, -1);
    updateCanvasCursor();
}

SceneWindow::~SceneWindow()
{
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

void SceneWindow::setSourceImage(const QList<QImage> & i)
{
    src_imp_img = transformed_img = QList<QImage>();
    current_frame = 0;
    scene() = Zx4ImageWithAttributes();
    if (!i.isEmpty())
    {
        scene().resize(i[0].width(), i[0].height());
        src_imp_img = transformed_img = (QList<QImage>() << i[0]);
    }
    if (widget)
    {
        widget->update();
    }
}

void SceneWindow::setImage(const QList<const Zx4Image *> & list)
{
    scene() = Zx4ImageWithAttributes();
    if (!list.isEmpty())
    {
        if (auto * attr_img = dynamic_cast<const Zx4ImageWithAttributes *>(list[0]))
        {
            scene() = *attr_img;
        }
    }
    if (widget)
    {
        widget->update();
    }
}

void SceneWindow::setAttr(Zx4ImageWithAttributes::Attr a)
{
    scene().attr = a;
    ui->canvas->update();
}

Zx4ImageWithAttributes::Attr SceneWindow::attr() const
{
    return scene().attr;
}

QPoint SceneWindow::currentChar() const
{
    return selected_char;
}

void SceneWindow::setAttr(QPoint p, int p_idx)
{
    if (p.x() >= 0 && p.y() >= 0)
    {
        scene().attr[p.x()][p.y()] = p_idx;
    }
}

void SceneWindow::setAttr(int p_idx)
{
    setAttr(selected_char, p_idx);
}

void SceneWindow::setPixel(QPoint p, int color_index)
{
    if (p.y() < 0 || p.y() >= scene().height() || p.x() < 0 || p.x() >= scene().width())
    {
        return;
    }
    scene().color_layer0[p.x()][p.y()] = (color_index == 0 || color_index == 1) ? Zx4Image::BitOff : Zx4Image::BitOn;
    scene().color_layer1[p.x()][p.y()] = (color_index == 0 || color_index == 2) ? Zx4Image::BitOff : Zx4Image::BitOn;
}

void SceneWindow::updateCanvasCursor()
{
    if (!ui->canvas)
    {
        return;
    }
    const Qt::KeyboardModifiers mods = QGuiApplication::keyboardModifiers();
    if (mods.testFlag(Qt::ControlModifier))
    {
        ui->canvas->setCursor(makeCursor(CursorPipette));
    }
    else if (mods.testFlag(Qt::ShiftModifier))
    {
        ui->canvas->setCursor(makeCursor(CursorStamp));
    }
    else if (selected_char != QPoint(-1, -1))
    {
        ui->canvas->setCursor(makeCursor(CursorPencil));
    }
    else
    {
        ui->canvas->setCursor(Qt::ArrowCursor);
    }
}

bool SceneWindow::eventFilter(QObject * o, QEvent * ev)
{
    if (o == ui->canvas)
    {
        switch (ev->type())
        {
        case QEvent::Enter:
            updateCanvasCursor();
            break;
        case QEvent::Paint:
            drawGrid();
            break;
        case QEvent::Wheel:
        {
            on_change = nullptr;
            QWheelEvent * we = dynamic_cast<QWheelEvent *>(ev);
            if (we && (we->modifiers().testFlag(Qt::ShiftModifier) || we->modifiers().testFlag(Qt::ControlModifier)))
            {
                if (selected_char != QPoint(-1, -1))
                {
                    int p_idx = scene().attr[selected_char.x()][selected_char.y()];
                    p_idx = std::clamp(p_idx + int(we->angleDelta().y() / 120), 0, int(palettes.palettes.count() - 1));
                    scene().attr[selected_char.x()][selected_char.y()] = p_idx;
                    importChar(
                        selected_char,
                        p_idx,
                        transformed_img[current_frame].copy(selected_char.x() * 8, selected_char.y() * 8, 8, 8));
                    emit charSelected(p_idx);
                    QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);
                    we->accept();
                    storeToHistory("Palette change at character position");
                    emit undoRedoMenuUpdateIsNeeded();
                    return true;
                }
            }
            break;
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonDblClick:
        {
            updateCanvasCursor();
            on_change = nullptr;
            QMouseEvent * me = dynamic_cast<QMouseEvent *>(ev);
            if (!me || transformed_img.isEmpty() || transformed_img[current_frame].isNull())
            {
                break;
            }
            const double step = qMax(0.01, qMin(1.0 * grid_x / transformed_img[current_frame].width(),
                                                1.0 * grid_y / transformed_img[current_frame].height()));
            const QPoint pt = me->position().toPoint();
            const int x = std::clamp(int(pt.x() / step), 0, transformed_img[current_frame].width() - 1);
            const int y = std::clamp(int(pt.y() / step), 0, transformed_img[current_frame].height() - 1);
            if (me->buttons().testFlag(Qt::LeftButton))
            {
                if (me->modifiers().testFlag(Qt::ShiftModifier))
                {
                    if (selected_char != QPoint(-1, -1))
                    {
                        update_char = QPoint(x / 8, y / 8);
                        importChar(
                            update_char,
                            scene().attr[selected_char.x()][selected_char.y()],
                            transformed_img[current_frame].copy(update_char.x() * 8, update_char.y() * 8, 8, 8));
                    }
                    on_change = [this]()
                    {
                        storeToHistory("Palette change at character position");
                        emit undoRedoMenuUpdateIsNeeded();
                    };
                }
                else if (me->modifiers().testFlag(Qt::ControlModifier))
                {
                    if (me->type() == QMouseEvent::MouseButtonDblClick && selected_char == QPoint(x / 8, y / 8))
                    {
                        selected_char = QPoint(-1, -1);
                    }
                    else
                    {
                        selected_char = QPoint(x / 8, y / 8);
                        emit charSelected(scene().attr[selected_char.x()][selected_char.y()]);
                    }
                    updateCanvasCursor();
                }
                else if (me->modifiers().testFlag(Qt::NoModifier))
                {
                    if (selected_char == QPoint(x / 8, y / 8))
                    {
                        setPixel(QPoint(x, y), currentColorIndex());
                        on_change = [this]()
                        {
                            storeToHistory("Pixel draw");
                            emit undoRedoMenuUpdateIsNeeded();
                        };
                    }
                }
                QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            updateCanvasCursor();
            if (on_change)
            {
                on_change();
                on_change = nullptr;
            }
            update_char = QPoint(-1, -1);
            QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(o, ev);
}

void SceneWindow::drawGrid()
{
    if (transformed_img.isEmpty() || transformed_img[current_frame].isNull())
    {
        return;
    }
    scene().resize(transformed_img[current_frame].width(), transformed_img[current_frame].height());
    rdr_img = QImage(transformed_img[current_frame].width(), transformed_img[current_frame].height(), QImage::Format_RGB32);
    for (int y = 0; y < rdr_img.height(); y++)
    {
        for (int x = 0; x < rdr_img.width(); x++)
        {
            int p_idx = std::clamp(scene().attr[x / 8][y / 8], 0, int(palettes.palettes.count() - 1));
            const auto & c = palettes.palettes[p_idx];
            char l0 = scene().color_layer0[x][y];
            char l1 = scene().color_layer1[x][y];
            rdr_img.setPixelColor(x, y, !l0 && !l1 ? c[0] : l0 && !l1 ? c[2] : !l0 && l1 ? c[1] : c[3]);
        }
    }
    QImage out = rdr_img.scaled(ui->canvas->size(), Qt::KeepAspectRatio);
    grid_x = out.width();
    grid_y = out.height();
    QPainter p(ui->canvas);
    p.fillRect(ui->canvas->rect(), Qt::black);
    p.drawImage(0, 0, out);

    double kx = 8.0 * grid_x / transformed_img[current_frame].width();
    double ky = 8.0 * grid_y / transformed_img[current_frame].height();
    const QList<QPair<QPoint, QColor>> list = {
        { selected_char, QColor(0, 255, 0, 128) },
        { update_char,   QColor(255, 0, 0, 128) }
    };
    const int pen_width = 3;
    for (const auto & item : list)
    {
        if (item.first != QPoint(-1, -1))
        {
            p.setPen(QPen(item.second, pen_width, Qt::DotLine, Qt::RoundCap));
            p.drawRect(item.first.x() * kx - pen_width / 2.0, item.first.y() * ky - pen_width / 2.0, kx + pen_width, ky + pen_width);
        }
    }
    QMetaObject::invokeMethod(this, "previewUpdateIsNeeded", Qt::QueuedConnection);
}

int SceneWindow::selectPalette(QImage small_quad, Zx4Palette::SelectPaletteMethod method, int dist_colors_criteria) const
{
    if (small_quad.isNull())
    {
        return -1;
    }

    struct Cluster {
        int r, g, b;
        int weight;
        int max_c, chroma;
    };

    QMap<QRgb, int> raw_counts;
    int dark_pixels_weight = 0;
    int total_pixels = 0;
    int min_luma = 255, max_luma = 0;

    for (int y = 0; y < small_quad.height(); ++y)
    {
        for (int x = 0; x < small_quad.width(); ++x)
        {
            QRgb px = small_quad.pixel(x, y);
            int r = qRed(px), g = qGreen(px), b = qBlue(px);
            int luma = (r * 77 + g * 150 + b * 29) >> 8;
            if (luma < min_luma) min_luma = luma;
            if (luma > max_luma) max_luma = luma;

            int w = (x < 8 && y < 8) ? 3 : 1;
            total_pixels += w;

            if (qMax(r, qMax(g, b)) < 36)
            {
                dark_pixels_weight += w;
            }

            int qr = (r & 0xF8) | (r >> 5);
            int qg = (g & 0xF8) | (g >> 5);
            int qb = (b & 0xF8) | (b >> 5);
            raw_counts[qRgb(qr, qg, qb)] += w;
        }
    }

    if (raw_counts.isEmpty())
    {
        return -1;
    }

    QVector<Cluster> clusters;
    clusters.reserve(raw_counts.size());
    for (auto it = raw_counts.cbegin(); it != raw_counts.cend(); ++it)
    {
        int r = qRed(it.key()), g = qGreen(it.key()), b = qBlue(it.key());
        int mx = qMax(r, qMax(g, b));
        int mn = qMin(r, qMin(g, b));
        clusters.push_back({ r, g, b, it.value(), mx, mx - mn });
    }

    const bool has_real_contrast = (max_luma - min_luma >= 45) && (dark_pixels_weight < total_pixels);
    const int effective_criteria = has_real_contrast ? dist_colors_criteria : 0;

    int best_p_idx = -1;
    double best_score = 1e18;

    for (int pal_idx = 0; pal_idx < palettes.palettes.count(); ++pal_idx)
    {
        const QList<QColor> & pal = palettes.palettes[pal_idx];

        bool pal_has_black = false;
        for (int s = 0; s < 4; ++s)
        {
            if (pal[s].red() == 0 && pal[s].green() == 0 && pal[s].blue() == 0)
            {
                pal_has_black = true;
                break;
            }
        }

        double total_err = 0.0;
        int used_mask = 0;

        for (const auto & cl : clusters)
        {
            double min_d = 1e18;
            int chosen_slot = 0;

            for (int slot = 0; slot < 4; ++slot)
            {
                const QColor & pc = pal[slot];
                const int cr = pc.red(), cg = pc.green(), cb = pc.blue();
                const int c_max = qMax(cr, qMax(cg, cb));
                const int c_min = qMin(cr, qMin(cg, cb));
                const int c_chroma = c_max - c_min;

                double d = 0.0;
                if (method == Zx4Palette::SelectPaletteCIE76)
                    d = RGB_color_Lab_difference_CIE76(cr, cg, cb, cl.r, cl.g, cl.b);
                else if (method == Zx4Palette::SelectPaletteCIE94)
                    d = RGB_color_Lab_difference_CIE94(cr, cg, cb, cl.r, cl.g, cl.b);
                else if (method == Zx4Palette::SelectPaletteCIEDE2000)
                    d = RGB_color_Lab_difference_CIEDE2000(cr, cg, cb, cl.r, cl.g, cl.b);
                else
                {
                    double dr = cr - cl.r, dg = cg - cl.g, db = cb - cl.b;
                    d = std::sqrt(dr * dr * 0.30 + dg * dg * 0.59 + db * db * 0.11) * 0.5;
                }

                double luma_diff = qAbs((cl.r * 77 + cl.g * 150 + cl.b * 29) - (cr * 77 + cg * 150 + cb * 29)) / 256.0;
                d += luma_diff * 0.14;

                if (cl.chroma > 18 && c_chroma == 0 && c_max > 0)
                {
                    d += cl.chroma * 0.30;
                }

                if (d < min_d)
                {
                    min_d = d;
                    chosen_slot = slot;
                }
            }

            used_mask |= (1 << chosen_slot);
            total_err += min_d * cl.weight;
        }

        if (dark_pixels_weight > 0 && !pal_has_black)
        {
            total_err += dark_pixels_weight * 45.0;
        }

        int unique_used = ((used_mask & 1) != 0) + ((used_mask & 2) != 0) + ((used_mask & 4) != 0) + ((used_mask & 8) != 0);
        if (effective_criteria > 0 && unique_used <= effective_criteria)
        {
            total_err *= 1.35;
        }

        if (total_err < best_score)
        {
            best_score = total_err;
            best_p_idx = pal_idx;
        }
    }

    return best_p_idx;
}

bool SceneWindow::checkPalette(QImage small_quad, int p_idx, int dist_colors_criteria) const
{
    if (p_idx == -1)
    {
        return false;
    }
    if (dist_colors_criteria <= 0)
    {
        return true;
    }
    const QList<QColor> & pal = palettes.palettes[p_idx];
    int used_mask = 0;
    for (int y = 0; y < small_quad.height(); ++y)
    {
        for (int x = 0; x < small_quad.width(); ++x)
        {
            QRgb px = small_quad.pixel(x, y);
            int pr = qRed(px), pg = qGreen(px), pb = qBlue(px);
            int best_slot = 0;
            double best_d = 1e18;
            for (int i = 0; i < 4; ++i)
            {
                double d = RGB_color_Lab_difference_CIE76(pal[i].red(), pal[i].green(), pal[i].blue(), pr, pg, pb);
                if (d < best_d)
                {
                    best_d = d;
                    best_slot = i;
                }
            }
            used_mask |= (1 << best_slot);
        }
    }
    int unique_count = ((used_mask & 1) != 0) + ((used_mask & 2) != 0) + ((used_mask & 4) != 0) + ((used_mask & 8) != 0);
    return unique_count > dist_colors_criteria;
}

Zx4ImageWithAttributes & SceneWindow::scene() const
{
    return *dynamic_cast<Zx4ImageWithAttributes *>(zx4_img.first().data());
}

void SceneWindow::importChar(QPoint char_pos, int p_idx, QImage small_quad)
{
    if (p_idx < 0)
    {
        return;
    }
    small_quad = Zx4Processing::importImage(small_quad.convertToFormat(QImage::Format_RGB32), palettes.colorTable(p_idx));
#pragma omp critical
    {
        if (!scene().isNull())
        {
            scene().attr[char_pos.x()][char_pos.y()] = p_idx;
            for (int iy = 0; iy < 8; iy++)
            {
                int y = iy + char_pos.y() * 8;
                if (y < transformed_img[current_frame].height())
                {
                    for (int ix = 0; ix < 8; ix++)
                    {
                        int x = ix + char_pos.x() * 8;
                        if (x < transformed_img[current_frame].width())
                        {
                            int idx = small_quad.pixelIndex(ix, iy);
                            if (idx >= 0 && idx < 4)
                            {
                                scene().color_layer0[x][y] = (idx == 0 || idx == 1) ? Zx4Image::BitOff : Zx4Image::BitOn;
                                scene().color_layer1[x][y] = (idx == 0 || idx == 2) ? Zx4Image::BitOff : Zx4Image::BitOn;
                                scene().alpha_layer[x][y] = Zx4Image::BitOn;
                            }
                        }
                    }
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
    for (int y = selected_char.y() * 8; y < qMin((selected_char.y() + 1) * 8, scene().height()); y++)
    {
        for (int x = selected_char.x() * 8; x < qMin((selected_char.x() + 1) * 8, scene().width()); x++)
        {
            scene().color_layer0[x][y] = scene().color_layer0[x][y] ? Zx4Image::BitOff : Zx4Image::BitOn;
            scene().color_layer1[x][y] = scene().color_layer1[x][y] ? Zx4Image::BitOff : Zx4Image::BitOn;
        }
    }
    ui->canvas->update();
    storeToHistory("Colors inversion");
    emit undoRedoMenuUpdateIsNeeded();
}
