#include <QMouseEvent>
#include <QWheelEvent>
#include <QAction>
#include <QToolButton>
#include <QScrollBar>
#include <QPainter>
#include <QFileInfo>
#include <QTimer>
#include <QVector>
#include <QApplication>
#include <QGuiApplication>
#include <algorithm>

#include "spritewindow.h"
#include "ui_spritewindow.h"

namespace {
constexpr int kStripCellSize = 52;
constexpr int kStripThumbSize = 44;
constexpr int kStripSpacing = 6;
constexpr int kStripMargin = 4;
}

SpriteWindow::SpriteWindow(const Zx4Palette & palettes, QWidget *parent) :
    QWidget(parent), Zx4BaseObject(palettes),
    ui(new Ui::SpriteWindow)
{
    ui->setupUi(this);
    setCanvas(ui->canvas);

    // Оформление кнопок + и - в тулбаре кадров
    for (QAction * act : { ui->actionAddFrame, ui->actionRemoveFrame })
    {
        if (QToolButton * btn = qobject_cast<QToolButton *>(ui->toolBar->widgetForAction(act)))
        {
            QFont f = btn->font();
            f.setBold(true);
            f.setPointSize(qMax(11, f.pointSize() + 2));
            btn->setFont(f);
            btn->setFixedSize(36, 36);
        }
    }

    // Полоска превью кадров с горизонтальным скроллом
    frame_scroll = new QScrollArea(ui->toolBar);
    frame_scroll->setWidgetResizable(false);
    frame_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    frame_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frame_scroll->setFrameShape(QFrame::StyledPanel);
    frame_scroll->setFixedHeight(kStripCellSize + kStripMargin * 2 + 16);
    frame_scroll->setMinimumWidth(100);
    frame_scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    frame_strip = new QWidget(frame_scroll);
    frame_strip->setMouseTracking(true);
    frame_strip->installEventFilter(this);
    frame_scroll->setWidget(frame_strip);

    ui->toolBar->addWidget(frame_scroll);

    clrs << Qt::black << Qt::black << Qt::black << Qt::black;
    setFileName(QString());
    updateFrameStrip();
    updateCanvasCursor();
}

SpriteWindow::~SpriteWindow()
{
    delete ui;
}

void SpriteWindow::setFileName(QString s)
{
    ui->actionAnimate->setChecked(false);
    Zx4BaseObject::setFileName(s);
    setWindowTitle((!file_name.isEmpty() ? QFileInfo(file_name).fileName() : "untitled") + " - Sprite");
}

void SpriteWindow::setSourceImage(const QList<QImage> & i)
{
    ui->actionAnimate->setChecked(false);
    Zx4BaseObject::setSourceImage(i);
    updateFrameStrip();
}

void SpriteWindow::setImage(const QList<const Zx4Image *> & i)
{
    ui->actionAnimate->setChecked(false);
    Zx4BaseObject::setImage(i);
    updateFrameStrip();
}

void SpriteWindow::onHistoryRestored()
{
    updateFrameStrip();
    ensureFrameVisible(current_frame);
}

QSize SpriteWindow::sizeHint() const
{
    return QSize(640, 640);
}

void SpriteWindow::setAction(ActionEnum e, QAction * a)
{
    acts[e] = a;
}

void SpriteWindow::setBrushSize(int s)
{
    if (s == 1 || s == 3 || s == 5 || s == 7)
    {
        brush_size = s;
    }
}

int SpriteWindow::brushSize() const
{
    return brush_size;
}

void SpriteWindow::setEraserSize(int s)
{
    if (s == 1 || s == 3 || s == 5 || s == 7)
    {
        eraser_size = s;
    }
}

int SpriteWindow::eraserSize() const
{
    return eraser_size;
}

void SpriteWindow::setColors(QList<QColor> _colors)
{
    if (_colors.count() != 4)
    {
        return;
    }
    clrs = _colors;
    ui->canvas->update();
    if (frame_strip)
    {
        frame_strip->update();
    }
}

QList<QColor> SpriteWindow::colors() const
{
    return clrs;
}

void SpriteWindow::setPaletteIndex(int i)
{
    p_idx = i;
}

int SpriteWindow::paletteIndex() const
{
    return p_idx;
}

void SpriteWindow::clearCanvas(QSize s)
{
    grid_x = s.width();
    grid_y = s.height();
    for (int k = 0; k < zx4_img.count(); k++)
    {
        zx4_img[k]->clear(grid_x, grid_y);
    }
    updateFrameStrip();
    ui->canvas->update();
}

void SpriteWindow::setCanvasSize(QSize s)
{
    grid_x = s.width();
    grid_y = s.height();
    for (int k = 0; k < zx4_img.count(); k++)
    {
        zx4_img[k]->resize(grid_x, grid_y);
    }
    updateFrameStrip();
    ui->canvas->update();
}

QSize SpriteWindow::canvasSize() const
{
    return QSize(grid_x, grid_y);
}

void SpriteWindow::resizeImage(QSize s)
{
    for (int i = 0; i < zx4_img.count(); ++i)
    {
        int new_image_x = s.width();
        int new_image_y = s.height();
        stored_sprite = *zx4_img[i];
        int old_grid_x = grid_x;
        if (new_image_x > old_grid_x)
        {
            grid_x = new_image_x;
        }
        zx4_img[i]->resize(grid_x, grid_y);
        if (new_image_x > old_grid_x)
        {
            for (int y = 0; y < grid_y; y++)
            {
                for (int x = 0; x < grid_x; x++)
                {
                    zx4_img[i]->color_layer0[x][y] = stored_sprite.color_layer0[x * old_grid_x / grid_x][y];
                    zx4_img[i]->color_layer1[x][y] = stored_sprite.color_layer1[x * old_grid_x / grid_x][y];
                    zx4_img[i]->alpha_layer[x][y] = stored_sprite.alpha_layer[x * old_grid_x / grid_x][y];
                }
            }
        }
        else if (new_image_x < old_grid_x)
        {
            for (int y = 0; y < grid_y; y++)
            {
                int x = 0;
                for (; x < new_image_x; x++)
                {
                    zx4_img[i]->color_layer0[x][y] = stored_sprite.color_layer0[x * old_grid_x / new_image_x][y];
                    zx4_img[i]->color_layer1[x][y] = stored_sprite.color_layer1[x * old_grid_x / new_image_x][y];
                    zx4_img[i]->alpha_layer[x][y] = stored_sprite.alpha_layer[x * old_grid_x / new_image_x][y];
                }
                for (; x < grid_x; x++)
                {
                    zx4_img[i]->alpha_layer[x][y] = Zx4Image::BitOff;
                }
            }
        }
        stored_sprite = *zx4_img[i];
        int old_grid_y = grid_y;
        if (new_image_y > old_grid_y)
        {
            grid_y = new_image_y;
        }
        zx4_img[i]->resize(grid_x, grid_y);
        if (new_image_y > old_grid_y)
        {
            for (int y = 0; y < grid_y; y++)
            {
                for (int x = 0; x < grid_x; x++)
                {
                    zx4_img[i]->color_layer0[x][y] = stored_sprite.color_layer0[x][y * old_grid_y / grid_y];
                    zx4_img[i]->color_layer1[x][y] = stored_sprite.color_layer1[x][y * old_grid_y / grid_y];
                    zx4_img[i]->alpha_layer[x][y] = stored_sprite.alpha_layer[x][y * old_grid_y / grid_y];
                }
            }
        }
        else if (new_image_y < old_grid_y)
        {
            int y = 0;
            for (; y < new_image_y; y++)
            {
                for (int x = 0; x < grid_x; x++)
                {
                    zx4_img[i]->color_layer0[x][y] = stored_sprite.color_layer0[x][y * old_grid_y / new_image_y];
                    zx4_img[i]->color_layer1[x][y] = stored_sprite.color_layer1[x][y * old_grid_y / new_image_y];
                    zx4_img[i]->alpha_layer[x][y] = stored_sprite.alpha_layer[x][y * old_grid_y / new_image_y];
                }
            }
            for (; y < grid_y; y++)
            {
                for (int x = 0; x < grid_x; x++)
                {
                    zx4_img[i]->alpha_layer[x][y] = Zx4Image::BitOff;
                }
            }
        }
    }
    updateFrameStrip();
    ui->canvas->update();
}

void SpriteWindow::updateCanvasCursor()
{
    if (!ui->canvas)
    {
        return;
    }
    if (ui->actionAnimate->isChecked())
    {
        ui->canvas->setCursor(Qt::ArrowCursor);
        return;
    }

    const Qt::KeyboardModifiers mods = QGuiApplication::keyboardModifiers();
    if (mods.testFlag(Qt::ControlModifier))
    {
        ui->canvas->setCursor(makeCursor(CursorPipette));
        return;
    }
    if (mods.testFlag(Qt::ShiftModifier))
    {
        ui->canvas->setCursor(makeCursor(CursorStamp));
        return;
    }

    if (QGuiApplication::mouseButtons().testFlag(Qt::RightButton) ||
        (acts[ActionEraser] && acts[ActionEraser]->isChecked()))
    {
        ui->canvas->setCursor(makeCursor(CursorEraser));
    }
    else if (acts[ActionBrush] && acts[ActionBrush]->isChecked())
    {
        ui->canvas->setCursor(makeCursor(CursorBrush));
    }
    else if (acts[ActionFill] && acts[ActionFill]->isChecked())
    {
        ui->canvas->setCursor(makeCursor(CursorFill));
    }
    else if (acts[ActionMove] && acts[ActionMove]->isChecked())
    {
        ui->canvas->setCursor(makeCursor(CursorMove));
    }
    else
    {
        ui->canvas->setCursor(makeCursor(CursorPencil));
    }
}

QRect SpriteWindow::frameCellRect(int index) const
{
    const int x = kStripMargin + index * (kStripCellSize + kStripSpacing);
    return QRect(x, kStripMargin, kStripCellSize, kStripCellSize);
}

int SpriteWindow::frameIndexAtPos(const QPoint & pos) const
{
    if (zx4_img.isEmpty())
    {
        return -1;
    }
    const int step = kStripCellSize + kStripSpacing;
    const int idx = (pos.x() - kStripMargin + kStripSpacing / 2) / step;
    return std::clamp(idx, 0, int(zx4_img.count() - 1));
}

QImage SpriteWindow::renderFrameThumbnail(int frame_idx, const QSize & box_size) const
{
    QImage thumb(box_size, QImage::Format_ARGB32);
    thumb.fill(Qt::transparent);
    if (frame_idx < 0 || frame_idx >= zx4_img.count() || zx4_img[frame_idx]->isNull())
    {
        return thumb;
    }

    const Zx4Image & frame = *zx4_img[frame_idx];
    const int fw = qMax(1, frame.width());
    const int fh = qMax(1, frame.height());
    QImage raw(fw, fh, QImage::Format_ARGB32);
    for (int y = 0; y < fh; ++y)
    {
        for (int x = 0; x < fw; ++x)
        {
            if (frame.alpha_layer[x][y])
            {
                raw.setPixelColor(x, y, clrs[colorIndex(frame, x, y)]);
            }
            else
            {
                const int checker = ((x / 4 + y / 4) % 2 == 0) ? 38 : 22;
                raw.setPixelColor(x, y, QColor(checker, checker, checker));
            }
        }
    }

    QImage scaled = raw.scaled(box_size, Qt::KeepAspectRatio, Qt::FastTransformation);
    QPainter p(&thumb);
    p.fillRect(thumb.rect(), QColor(25, 25, 25));
    p.drawImage((box_size.width() - scaled.width()) / 2,
                (box_size.height() - scaled.height()) / 2,
                scaled);
    return thumb;
}

void SpriteWindow::updateFrameStrip()
{
    if (!frame_strip)
    {
        return;
    }
    const int count = qMax(1, int(zx4_img.count()));
    current_frame = std::clamp(current_frame, 0, count - 1);

    ui->actionRemoveFrame->setEnabled(zx4_img.count() > 1);
    ui->actionAddFrame->setEnabled(zx4_img.count() < 255);

    const int total_w = kStripMargin * 2 + count * kStripCellSize + (count - 1) * kStripSpacing;
    const int total_h = kStripMargin * 2 + kStripCellSize;
    frame_strip->setFixedSize(total_w, total_h);
    frame_strip->update();
}

void SpriteWindow::ensureFrameVisible(int index)
{
    if (!frame_scroll || index < 0 || index >= zx4_img.count())
    {
        return;
    }
    const QRect r = frameCellRect(index);
    frame_scroll->ensureVisible(r.center().x(), r.center().y(), kStripCellSize, kStripCellSize / 2);
}

void SpriteWindow::selectFrame(int index)
{
    if (zx4_img.isEmpty())
    {
        return;
    }
    index = std::clamp(index, 0, int(zx4_img.count() - 1));
    setCurrentFrame(index);
    updateFrameStrip();
    ensureFrameVisible(current_frame);
    ui->canvas->update();
    emit previewUpdateIsNeeded();
}

void SpriteWindow::drawFrameStrip()
{
    QPainter p(frame_strip);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(frame_strip->rect(), QColor(32, 32, 32));

    const int count = zx4_img.count();
    for (int i = 0; i < count; ++i)
    {
        const QRect cell = frameCellRect(i);
        const bool is_selected = (i == current_frame);
        const bool is_dragged = (is_dragging_frame && i == drag_source_frame);

        p.setPen(Qt::NoPen);
        p.setBrush(is_selected ? QColor(50, 95, 150) : QColor(45, 45, 45));
        p.drawRoundedRect(cell, 4, 4);

        const QSize thumb_sz(kStripThumbSize, kStripThumbSize);
        const QImage thumb = renderFrameThumbnail(i, thumb_sz);
        p.drawImage(cell.x() + (cell.width() - thumb_sz.width()) / 2,
                    cell.y() + (cell.height() - thumb_sz.height()) / 2,
                    thumb);

        // Рамка выделения активного/перетаскиваемого кадра
        if (is_dragged)
        {
            p.setPen(QPen(QColor(255, 215, 0), 2.5));
        }
        else if (is_selected)
        {
            p.setPen(QPen(QColor(42, 130, 218), 2.5));
        }
        else
        {
            p.setPen(QPen(QColor(80, 80, 80), 1.0));
        }
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(cell.adjusted(1, 1, -1, -1), 4, 4);

        // Бейдж с номером кадра
        const QString num_str = QString::number(i + 1);
        QFont f = p.font();
        f.setPointSize(7);
        f.setBold(true);
        p.setFont(f);
        const QRect badge_rect(cell.x() + 3, cell.y() + 3, 16, 12);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 175));
        p.drawRoundedRect(badge_rect, 2, 2);
        p.setPen(Qt::white);
        p.drawText(badge_rect, Qt::AlignCenter, num_str);
    }
}

bool SpriteWindow::eventFilter(QObject * o, QEvent * ev)
{
    if (o == frame_strip)
    {
        switch (ev->type())
        {
        case QEvent::Paint:
            drawFrameStrip();
            return true;

        case QEvent::Wheel:
        {
            if (auto * we = dynamic_cast<QWheelEvent *>(ev))
            {
                if (frame_scroll && frame_scroll->horizontalScrollBar())
                {
                    int delta = we->angleDelta().y() != 0 ? we->angleDelta().y() : we->angleDelta().x();
                    frame_scroll->horizontalScrollBar()->setValue(
                        frame_scroll->horizontalScrollBar()->value() - delta / 2);
                    we->accept();
                    return true;
                }
            }
            break;
        }

        case QEvent::MouseButtonPress:
        {
            auto * me = dynamic_cast<QMouseEvent *>(ev);
            if (me && me->button() == Qt::LeftButton && !zx4_img.isEmpty())
            {
                const int idx = frameIndexAtPos(me->position().toPoint());
                if (idx >= 0 && idx < zx4_img.count())
                {
                    ui->actionAnimate->setChecked(false);
                    drag_initial_frame = idx;
                    drag_source_frame = idx;
                    drag_start_pos = me->position().toPoint();
                    is_dragging_frame = false;
                    frame_strip->setCursor(Qt::PointingHandCursor);
                    selectFrame(idx);
                }
                return true;
            }
            break;
        }

        case QEvent::MouseMove:
        {
            auto * me = dynamic_cast<QMouseEvent *>(ev);
            if (!me)
            {
                break;
            }
            if (me->buttons().testFlag(Qt::LeftButton) && drag_source_frame >= 0)
            {
                if (!is_dragging_frame &&
                    (me->position().toPoint() - drag_start_pos).manhattanLength() >= QApplication::startDragDistance())
                {
                    is_dragging_frame = true;
                    frame_strip->setCursor(Qt::ClosedHandCursor);
                }

                if (is_dragging_frame)
                {
                    const int target_idx = frameIndexAtPos(me->position().toPoint());
                    if (target_idx >= 0 && target_idx < zx4_img.count() && target_idx != drag_source_frame)
                    {
                        zx4_img.move(drag_source_frame, target_idx);
                        if (src_imp_img.count() == zx4_img.count())
                        {
                            src_imp_img.move(drag_source_frame, target_idx);
                        }
                        if (transformed_img.count() == zx4_img.count())
                        {
                            transformed_img.move(drag_source_frame, target_idx);
                        }
                        drag_source_frame = target_idx;
                        selectFrame(target_idx);
                    }
                    else
                    {
                        frame_strip->update();
                    }
                }
                return true;
            }
            else
            {
                frame_strip->setCursor(zx4_img.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
            }
            break;
        }

        case QEvent::MouseButtonRelease:
        {
            if (drag_source_frame >= 0)
            {
                const bool order_changed = is_dragging_frame &&
                                           drag_initial_frame >= 0 &&
                                           drag_source_frame != drag_initial_frame;
                drag_initial_frame = -1;
                drag_source_frame = -1;
                is_dragging_frame = false;
                frame_strip->setCursor(Qt::PointingHandCursor);
                frame_strip->update();
                if (order_changed)
                {
                    storeToHistory("Reorder frames");
                    emit undoRedoMenuUpdateIsNeeded();
                }
                return true;
            }
            break;
        }

        default:
            break;
        }
    }
    else if (o == ui->canvas)
    {
        switch (ev->type())
        {
        case QEvent::Enter:
            updateCanvasCursor();
            break;
        case QEvent::Paint:
            drawGrid();
            break;
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
            updateCanvasCursor();
            on_change = nullptr;
            if (!ui->actionAnimate->isChecked())
            {
                QMouseEvent * me = dynamic_cast<QMouseEvent *>(ev);
                if (!me)
                {
                    break;
                }
                int step = qMax(1, qMin(ui->canvas->width() / grid_x, ui->canvas->height() / grid_y));
                QPoint pt = me->position().toPoint();
                int x = (pt.x() - 1) / step;
                int y = (pt.y() - 1) / step;
                if (me->buttons().testFlag(Qt::LeftButton))
                {
                    if (acts[ActionPencil] && acts[ActionPencil]->isChecked())
                    {
                        drawDot(x, y);
                        on_change = [=]()
                        {
                            storeToHistory("Pixel draw");
                            emit undoRedoMenuUpdateIsNeeded();
                        };
                    }
                    else if (acts[ActionBrush] && acts[ActionBrush]->isChecked())
                    {
                        drawBrush(x, y);
                        on_change = [=]()
                        {
                            storeToHistory("Brush draw");
                            emit undoRedoMenuUpdateIsNeeded();
                        };
                    }
                    else if (acts[ActionFill] && acts[ActionFill]->isChecked())
                    {
                        if (ev->type() == QEvent::MouseButtonPress)
                        {
                            floodFill(x, y);
                            on_change = [=]()
                            {
                                storeToHistory("Flood fill");
                                emit undoRedoMenuUpdateIsNeeded();
                            };
                        }
                    }
                    else if (acts[ActionMove] && acts[ActionMove]->isChecked())
                    {
                        if (ev->type() == QEvent::MouseButtonPress)
                        {
                            stored_sprite = *zx4_img[current_frame];
                            old_grid_pos = QPoint(x, y);
                        }
                        else if (old_grid_pos != QPoint(x, y))
                        {
                            int dx = old_grid_pos.x() - x;
                            int dy = old_grid_pos.y() - y;
                            moveImage(dx, dy);
                            on_change = [=]()
                            {
                                storeToHistory("Sprite move");
                                emit undoRedoMenuUpdateIsNeeded();
                            };
                        }
                    }
                    QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);
                }
                if (me->buttons().testFlag(Qt::RightButton) || (me->buttons().testFlag(Qt::LeftButton) && acts[ActionEraser] && acts[ActionEraser]->isChecked()))
                {
                    eraseBrush(x, y);
                    QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);
                    on_change = [=]()
                    {
                        storeToHistory("Pixel erase");
                        emit undoRedoMenuUpdateIsNeeded();
                    };
                }
            }
            break;
        case QEvent::MouseButtonRelease:
            updateCanvasCursor();
            if (on_change)
            {
                on_change();
                on_change = nullptr;
            }
            break;
        default:
            break;
        }
    }
    return QWidget::eventFilter(o, ev);
}

int SpriteWindow::colorIndex(const Zx4Image & img, int x, int y) const
{
    if (!img.color_layer0[x][y] && !img.color_layer1[x][y])
    {
        return 0;
    }
    else if (!img.color_layer0[x][y] && img.color_layer1[x][y])
    {
        return 1;
    }
    else if (img.color_layer0[x][y] && !img.color_layer1[x][y])
    {
        return 2;
    }
    return 3;
}

void SpriteWindow::drawPixels(QPainter * p, int step)
{
    for (int y = 0; y < grid_y; y++)
    {
        for (int x = 0; x < grid_x; x++)
        {
            if (zx4_img[current_frame]->alpha_layer[x][y])
            {
                p->fillRect(x * step, y * step, step, step, clrs[colorIndex(*zx4_img[current_frame], x, y)]);
            }
        }
    }
}

void SpriteWindow::drawGrid()
{
    if (zx4_img.isEmpty())
    {
        return;
    }
    current_frame = std::clamp(current_frame, 0, int(zx4_img.count() - 1));
    zx4_img[current_frame]->resize(grid_x, grid_y);
    int step = qMax(1, qMin((ui->canvas->width() - 1) / grid_x, (ui->canvas->height() - 1) / grid_y));
    int side_x = step * grid_x;
    int side_y = step * grid_y;
    QColor grid_color(55, 55, 55);
    QPainter p(ui->canvas);
    p.fillRect(ui->canvas->rect(), Qt::black);
    p.fillRect(ui->canvas->rect(), QBrush(grid_color, Qt::Dense6Pattern));
    drawPixels(&p, step);
    p.setPen(grid_color);
    for (int y = 0; y <= grid_y; y++)
    {
        p.drawLine(0, y * step, side_x, y * step);
    }
    for (int x = 0; x <= grid_x; x++)
    {
        p.drawLine(x * step, 0, x * step, side_y);
    }
    if (frame_strip)
    {
        frame_strip->update();
    }
    QMetaObject::invokeMethod(this, "previewUpdateIsNeeded", Qt::QueuedConnection);
}

void SpriteWindow::drawDot(int x, int y)
{
    if (x < 0 || y < 0 || x >= grid_x || y >= grid_y)
    {
        return;
    }
    zx4_img[current_frame]->color_layer0[x][y] = (current_color_index == 2 || current_color_index == 3) ? Zx4Image::BitOn : Zx4Image::BitOff;
    zx4_img[current_frame]->color_layer1[x][y] = (current_color_index == 1 || current_color_index == 3) ? Zx4Image::BitOn : Zx4Image::BitOff;
    zx4_img[current_frame]->alpha_layer[x][y] = Zx4Image::BitOn;
}

void SpriteWindow::eraseDot(int x, int y)
{
    if (x < 0 || y < 0 || x >= grid_x || y >= grid_y)
    {
        return;
    }
    zx4_img[current_frame]->alpha_layer[x][y] = Zx4Image::BitOff;
}

void SpriteWindow::drawBrush(int x, int y)
{
    const int r = brush_size / 2;
    for (int dy = -r; dy <= r; ++dy)
    {
        for (int dx = -r; dx <= r; ++dx)
        {
            if (brush_size == 5 && qAbs(dx) == 2 && qAbs(dy) == 2)
            {
                continue;
            }
            if (brush_size == 7 && (dx * dx + dy * dy > 11))
            {
                continue;
            }
            drawDot(x + dx, y + dy);
        }
    }
}

void SpriteWindow::eraseBrush(int x, int y)
{
    const int r = eraser_size / 2;
    for (int dy = -r; dy <= r; ++dy)
    {
        for (int dx = -r; dx <= r; ++dx)
        {
            if (eraser_size == 5 && qAbs(dx) == 2 && qAbs(dy) == 2)
            {
                continue;
            }
            if (eraser_size == 7 && (dx * dx + dy * dy > 11))
            {
                continue;
            }
            eraseDot(x + dx, y + dy);
        }
    }
}

void SpriteWindow::floodFill(int x, int y)
{
    if (x < 0 || y < 0 || x >= grid_x || y >= grid_y || zx4_img.isEmpty())
    {
        return;
    }
    Zx4Image & img = *zx4_img[current_frame];
    auto pixelState = [&](int px, int py) -> int
    {
        if (!img.alpha_layer[px][py])
        {
            return -1;
        }
        return colorIndex(img, px, py);
    };

    const int target_state = pixelState(x, y);
    const int replacement_state = current_color_index;
    if (target_state == replacement_state)
    {
        return;
    }

    QVector<QPoint> queue;
    queue.reserve(grid_x * grid_y);
    drawDot(x, y);
    queue.push_back(QPoint(x, y));

    int head = 0;
    while (head < queue.size())
    {
        const QPoint pt = queue[head++];
        const QPoint neighbors[4] = {
            QPoint(pt.x() - 1, pt.y()),
            QPoint(pt.x() + 1, pt.y()),
            QPoint(pt.x(), pt.y() - 1),
            QPoint(pt.x(), pt.y() + 1)
        };
        for (const QPoint & n : neighbors)
        {
            if (n.x() >= 0 && n.x() < grid_x && n.y() >= 0 && n.y() < grid_y)
            {
                if (pixelState(n.x(), n.y()) == target_state)
                {
                    drawDot(n.x(), n.y());
                    queue.push_back(n);
                }
            }
        }
    }
}

void SpriteWindow::moveImage(int dx, int dy)
{
    for (int my = 0; my < grid_y; my++)
    {
        for (int mx = 0; mx < grid_x; mx++)
        {
            if (my + dy >= 0 && my + dy < grid_y && mx + dx >= 0 && mx + dx < grid_x)
            {
                zx4_img[current_frame]->color_layer0[mx][my] = stored_sprite.color_layer0[mx + dx][my + dy];
                zx4_img[current_frame]->color_layer1[mx][my] = stored_sprite.color_layer1[mx + dx][my + dy];
                zx4_img[current_frame]->alpha_layer[mx][my] = stored_sprite.alpha_layer[mx + dx][my + dy];
            }
            else
            {
                zx4_img[current_frame]->color_layer0[mx][my] = Zx4Image::BitOff;
                zx4_img[current_frame]->color_layer1[mx][my] = Zx4Image::BitOff;
                zx4_img[current_frame]->alpha_layer[mx][my] = Zx4Image::BitOff;
            }
        }
    }
}

void SpriteWindow::drawImage(QImage img)
{
    if (img.isNull())
    {
        return;
    }
    grid_x = qMax(img.width(), grid_x);
    grid_y = qMax(img.height(), grid_y);
    zx4_img[current_frame]->clear(grid_x, grid_y);
    int saved_color_index = current_color_index;
    int x_offs = (grid_x - img.width()) / 2;
    int y_offs = (grid_y - img.height()) / 2;
    for (int y = 0; y < img.height(); y++)
    {
        for (int x = 0; x < img.width(); x++)
        {
            int idx = img.pixelIndex(x, y);
            if (idx >= 0 && idx < 4)
            {
                current_color_index = idx;
                drawDot(x_offs + x, y_offs + y);
            }
        }
    }
    current_color_index = saved_color_index;
    updateFrameStrip();
    ui->canvas->update();
}

void SpriteWindow::invertColors()
{
    if (zx4_img[current_frame]->isNull())
    {
        return;
    }
    for (int y = 0; y < grid_y; y++)
    {
        for (int x = 0; x < grid_x; x++)
        {
            zx4_img[current_frame]->color_layer0[x][y] = zx4_img[current_frame]->color_layer0[x][y] ? Zx4Image::BitOff : Zx4Image::BitOn;
            zx4_img[current_frame]->color_layer1[x][y] = zx4_img[current_frame]->color_layer1[x][y] ? Zx4Image::BitOff : Zx4Image::BitOn;
        }
    }
    updateFrameStrip();
    ui->canvas->update();
    storeToHistory("Colors inversion");
    emit undoRedoMenuUpdateIsNeeded();
}

void SpriteWindow::on_actionAnimate_toggled(bool state)
{
    updateCanvasCursor();
    if (state)
    {
        animationFrame();
    }
}

void SpriteWindow::on_actionStop_triggered()
{
    ui->actionAnimate->setChecked(false);
    selectFrame(0);
}

void SpriteWindow::on_actionAddFrame_triggered()
{
    if (zx4_img.isEmpty() || zx4_img.count() >= 255)
    {
        return;
    }
    ui->actionAnimate->setChecked(false);
    current_frame = std::clamp(current_frame, 0, int(zx4_img.count() - 1));
    const int insert_idx = current_frame + 1;

    zx4_img.insert(insert_idx, QSharedPointer<Zx4Image>(zx4_img[current_frame]->clone()));
    if (current_frame >= 0 && current_frame < src_imp_img.count())
    {
        src_imp_img.insert(insert_idx, src_imp_img[current_frame]);
    }
    if (current_frame >= 0 && current_frame < transformed_img.count())
    {
        transformed_img.insert(insert_idx, transformed_img[current_frame]);
    }

    selectFrame(insert_idx);
    storeToHistory("Add frame");
    emit undoRedoMenuUpdateIsNeeded();
}

void SpriteWindow::on_actionRemoveFrame_triggered()
{
    if (zx4_img.count() <= 1)
    {
        return;
    }
    ui->actionAnimate->setChecked(false);
    current_frame = std::clamp(current_frame, 0, int(zx4_img.count() - 1));

    zx4_img.removeAt(current_frame);
    if (current_frame >= 0 && current_frame < src_imp_img.count())
    {
        src_imp_img.removeAt(current_frame);
    }
    if (current_frame >= 0 && current_frame < transformed_img.count())
    {
        transformed_img.removeAt(current_frame);
    }

    selectFrame(std::clamp(current_frame, 0, int(zx4_img.count() - 1)));
    storeToHistory("Delete frame");
    emit undoRedoMenuUpdateIsNeeded();
}

void SpriteWindow::animationFrame()
{
    if (ui->actionAnimate->isChecked())
    {
        if (!zx4_img.isEmpty())
        {
            const int next = (current_frame + 1) % zx4_img.count();
            selectFrame(next);
        }
        QTimer::singleShot(200, this, &SpriteWindow::animationFrame);
    }
}
