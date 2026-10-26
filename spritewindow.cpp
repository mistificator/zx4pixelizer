#include <QMouseEvent>
#include <QWheelEvent>
#include <QAction>
#include <QToolButton>
#include <QScrollBar>
#include <QPainter>
#include <QFileInfo>
#include <QTimer>
#include <QVector>
#include <QLine>
#include <QMessageBox>
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

SpriteWindow * SpriteWindow::spriteWindowAtGlobalPos(const QPoint & globalPos)
{
    QWidget * w = QApplication::widgetAt(globalPos);
    while (w)
    {
        if (auto * sw = qobject_cast<SpriteWindow *>(w))
        {
            return sw;
        }
        w = w->parentWidget();
    }
    return nullptr;
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
    invalidateThumbnails();
    updateFrameStrip();
}

void SpriteWindow::setImage(const QList<const Zx4Image *> & i)
{
    ui->actionAnimate->setChecked(false);
    Zx4BaseObject::setImage(i);
    invalidateThumbnails();
    updateFrameStrip();
}

void SpriteWindow::onHistoryRestored()
{
    invalidateThumbnails();
    updateFrameStrip();
    ensureFrameVisible(current_frame);
    emit previewUpdateIsNeeded();
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
    if (_colors.count() != 4 || clrs == _colors)
    {
        return;
    }
    clrs = _colors;
    invalidateThumbnails();
    ui->canvas->update();
    if (frame_strip)
    {
        frame_strip->update();
    }
    emit previewUpdateIsNeeded();
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

void SpriteWindow::invalidateThumbnails(int frame_idx)
{
    if (frame_idx < 0)
    {
        thumb_cache.clear();
    }
    else if (frame_idx < thumb_cache.size())
    {
        thumb_cache[frame_idx] = QImage();
    }
}

void SpriteWindow::clearCanvas(QSize s)
{
    grid_x = s.width();
    grid_y = s.height();
    for (int k = 0; k < zx4_img.count(); k++)
    {
        zx4_img[k]->clear(grid_x, grid_y);
    }
    invalidateThumbnails();
    updateFrameStrip();
    ui->canvas->update();
    emit previewUpdateIsNeeded();
}

void SpriteWindow::setCanvasSize(QSize s)
{
    grid_x = s.width();
    grid_y = s.height();
    for (int k = 0; k < zx4_img.count(); k++)
    {
        zx4_img[k]->resize(grid_x, grid_y);
    }
    invalidateThumbnails();
    updateFrameStrip();
    ui->canvas->update();
    emit previewUpdateIsNeeded();
}

QSize SpriteWindow::canvasSize() const
{
    return QSize(grid_x, grid_y);
}

void SpriteWindow::resizeImage(QSize s)
{
    const int orig_grid_x = grid_x;
    const int orig_grid_y = grid_y;
    const int target_grid_x = qMax(orig_grid_x, s.width());
    const int target_grid_y = qMax(orig_grid_y, s.height());

    for (int i = 0; i < zx4_img.count(); ++i)
    {
        int new_image_x = s.width();
        int new_image_y = s.height();
        stored_sprite = *zx4_img[i];
        int old_grid_x = orig_grid_x;
        int cur_w = target_grid_x;
        zx4_img[i]->resize(cur_w, orig_grid_y);
        if (new_image_x > old_grid_x)
        {
            for (int y = 0; y < orig_grid_y; y++)
            {
                for (int x = 0; x < cur_w; x++)
                {
                    zx4_img[i]->color_layer0[x][y] = stored_sprite.color_layer0[x * old_grid_x / cur_w][y];
                    zx4_img[i]->color_layer1[x][y] = stored_sprite.color_layer1[x * old_grid_x / cur_w][y];
                    zx4_img[i]->alpha_layer[x][y] = stored_sprite.alpha_layer[x * old_grid_x / cur_w][y];
                }
            }
        }
        else if (new_image_x < old_grid_x)
        {
            for (int y = 0; y < orig_grid_y; y++)
            {
                int x = 0;
                for (; x < new_image_x; x++)
                {
                    zx4_img[i]->color_layer0[x][y] = stored_sprite.color_layer0[x * old_grid_x / new_image_x][y];
                    zx4_img[i]->color_layer1[x][y] = stored_sprite.color_layer1[x * old_grid_x / new_image_x][y];
                    zx4_img[i]->alpha_layer[x][y] = stored_sprite.alpha_layer[x * old_grid_x / new_image_x][y];
                }
                for (; x < cur_w; x++)
                {
                    zx4_img[i]->alpha_layer[x][y] = Zx4Image::BitOff;
                }
            }
        }
        stored_sprite = *zx4_img[i];
        int old_grid_y = orig_grid_y;
        int cur_h = target_grid_y;
        zx4_img[i]->resize(cur_w, cur_h);
        if (new_image_y > old_grid_y)
        {
            for (int y = 0; y < cur_h; y++)
            {
                for (int x = 0; x < cur_w; x++)
                {
                    zx4_img[i]->color_layer0[x][y] = stored_sprite.color_layer0[x][y * old_grid_y / cur_h];
                    zx4_img[i]->color_layer1[x][y] = stored_sprite.color_layer1[x][y * old_grid_y / cur_h];
                    zx4_img[i]->alpha_layer[x][y] = stored_sprite.alpha_layer[x][y * old_grid_y / cur_h];
                }
            }
        }
        else if (new_image_y < old_grid_y)
        {
            int y = 0;
            for (; y < new_image_y; y++)
            {
                for (int x = 0; x < cur_w; x++)
                {
                    zx4_img[i]->color_layer0[x][y] = stored_sprite.color_layer0[x][y * old_grid_y / new_image_y];
                    zx4_img[i]->color_layer1[x][y] = stored_sprite.color_layer1[x][y * old_grid_y / new_image_y];
                    zx4_img[i]->alpha_layer[x][y] = stored_sprite.alpha_layer[x][y * old_grid_y / new_image_y];
                }
            }
            for (; y < cur_h; y++)
            {
                for (int x = 0; x < cur_w; x++)
                {
                    zx4_img[i]->alpha_layer[x][y] = Zx4Image::BitOff;
                }
            }
        }
    }
    grid_x = target_grid_x;
    grid_y = target_grid_y;
    invalidateThumbnails();
    updateFrameStrip();
    ui->canvas->update();
    emit previewUpdateIsNeeded();
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
    thumb.fill(QColor(25, 25, 25));
    if (frame_idx < 0 || frame_idx >= zx4_img.count() || zx4_img[frame_idx]->isNull())
    {
        return thumb;
    }

    const Zx4Image & frame = *zx4_img[frame_idx];
    const int fw = qMax(1, frame.width());
    const int fh = qMax(1, frame.height());
    const QRgb c_rgb[4] = {
        clrs[0].rgba(),
        clrs[1].rgba(),
        clrs[2].rgba(),
        clrs[3].rgba()
    };

    QImage raw(fw, fh, QImage::Format_RGB32);
    for (int y = 0; y < fh; ++y)
    {
        QRgb * scan = reinterpret_cast<QRgb *>(raw.scanLine(y));
        for (int x = 0; x < fw; ++x)
        {
            if (frame.alpha_layer[x][y])
            {
                scan[x] = c_rgb[colorIndex(frame, x, y)];
            }
            else
            {
                const int checker = ((x / 4 + y / 4) % 2 == 0) ? 38 : 22;
                scan[x] = qRgb(checker, checker, checker);
            }
        }
    }

    QImage scaled = raw.scaled(box_size, Qt::KeepAspectRatio, Qt::FastTransformation);
    QPainter p(&thumb);
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
    if (thumb_cache.size() != zx4_img.count())
    {
        thumb_cache.resize(zx4_img.count());
    }

    ui->actionRemoveFrame->setEnabled(zx4_img.count() > 1);
    ui->actionAddFrame->setEnabled(zx4_img.count() < 255);

    const int total_w = kStripMargin * 2 + count * kStripCellSize + (count - 1) * kStripSpacing;
    const int total_h = kStripMargin * 2 + kStripCellSize;
    if (frame_strip->size() != QSize(total_w, total_h))
    {
        frame_strip->setFixedSize(total_w, total_h);
    }
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

bool SpriteWindow::insertFrameAt(int insert_idx, const Zx4Image & frame_img, const QString & history_desc)
{
    if (zx4_img.count() >= 255)
    {
        return false;
    }
    if (QSize(frame_img.width(), frame_img.height()) != canvasSize())
    {
        QMessageBox::warning(
            window(),
            "Size mismatch",
            QString("Cannot copy/paste frame: sprite sizes do not match (%1x%2 vs %3x%4).")
                .arg(frame_img.width())
                .arg(frame_img.height())
                .arg(canvasSize().width())
                .arg(canvasSize().height()));
        return false;
    }

    ui->actionAnimate->setChecked(false);
    insert_idx = std::clamp(insert_idx, 0, int(zx4_img.count()));

    zx4_img.insert(insert_idx, QSharedPointer<Zx4Image>(frame_img.clone()));
    if (!src_imp_img.isEmpty())
    {
        src_imp_img.insert(std::clamp(insert_idx, 0, int(src_imp_img.count())), QImage());
    }
    if (!transformed_img.isEmpty())
    {
        transformed_img.insert(std::clamp(insert_idx, 0, int(transformed_img.count())), QImage());
    }

    invalidateThumbnails();
    selectFrame(insert_idx);
    storeToHistory(history_desc);
    emit undoRedoMenuUpdateIsNeeded();
    return true;
}

void SpriteWindow::clearCurrentFrame(const QString & history_desc)
{
    if (zx4_img.isEmpty())
    {
        return;
    }
    ui->actionAnimate->setChecked(false);
    current_frame = std::clamp(current_frame, 0, int(zx4_img.count() - 1));
    zx4_img[current_frame]->clear(grid_x, grid_y);
    invalidateThumbnails(current_frame);
    updateFrameStrip();
    ui->canvas->update();
    emit previewUpdateIsNeeded();
    storeToHistory(history_desc);
    emit undoRedoMenuUpdateIsNeeded();
}

void SpriteWindow::cutCurrentFrame()
{
    if (zx4_img.isEmpty())
    {
        return;
    }
    ui->actionAnimate->setChecked(false);
    current_frame = std::clamp(current_frame, 0, int(zx4_img.count() - 1));

    if (zx4_img.count() > 1)
    {
        zx4_img.removeAt(current_frame);
        if (current_frame >= 0 && current_frame < src_imp_img.count())
        {
            src_imp_img.removeAt(current_frame);
        }
        if (current_frame >= 0 && current_frame < transformed_img.count())
        {
            transformed_img.removeAt(current_frame);
        }
        invalidateThumbnails();
        selectFrame(std::clamp(current_frame, 0, int(zx4_img.count() - 1)));
        storeToHistory("Cut frame");
        emit undoRedoMenuUpdateIsNeeded();
    }
    else
    {
        clearCurrentFrame("Cut frame");
    }
}

void SpriteWindow::drawFrameStrip()
{
    QPainter p(frame_strip);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(frame_strip->rect(), QColor(32, 32, 32));

    const int count = zx4_img.count();
    if (thumb_cache.size() != count)
    {
        thumb_cache.resize(count);
    }

    const QSize thumb_sz(kStripThumbSize, kStripThumbSize);
    for (int i = 0; i < count; ++i)
    {
        const QRect cell = frameCellRect(i);
        const bool is_selected = (i == current_frame);
        const bool is_dragged = (is_dragging_frame && i == drag_source_frame);

        p.setPen(Qt::NoPen);
        p.setBrush(is_selected ? QColor(50, 95, 150) : QColor(45, 45, 45));
        p.drawRoundedRect(cell, 4, 4);

        if (thumb_cache[i].isNull())
        {
            thumb_cache[i] = renderFrameThumbnail(i, thumb_sz);
        }
        p.drawImage(cell.x() + (cell.width() - thumb_sz.width()) / 2,
                    cell.y() + (cell.height() - thumb_sz.height()) / 2,
                    thumb_cache[i]);

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
                }

                if (is_dragging_frame)
                {
                    SpriteWindow * target_sprite = spriteWindowAtGlobalPos(me->globalPosition().toPoint());
                    if (target_sprite && target_sprite != this)
                    {
                        if (drag_initial_frame >= 0 && drag_source_frame != drag_initial_frame)
                        {
                            zx4_img.move(drag_source_frame, drag_initial_frame);
                            if (thumb_cache.size() == zx4_img.count())
                            {
                                thumb_cache.move(drag_source_frame, drag_initial_frame);
                            }
                            if (src_imp_img.count() == zx4_img.count())
                            {
                                src_imp_img.move(drag_source_frame, drag_initial_frame);
                            }
                            if (transformed_img.count() == zx4_img.count())
                            {
                                transformed_img.move(drag_source_frame, drag_initial_frame);
                            }
                            drag_source_frame = drag_initial_frame;
                            selectFrame(drag_initial_frame);
                        }
                        frame_strip->setCursor(Qt::DragCopyCursor);
                    }
                    else if (target_sprite == this)
                    {
                        frame_strip->setCursor(Qt::ClosedHandCursor);
                        const int target_idx = frameIndexAtPos(me->position().toPoint());
                        if (target_idx >= 0 && target_idx < zx4_img.count() && target_idx != drag_source_frame)
                        {
                            zx4_img.move(drag_source_frame, target_idx);
                            if (thumb_cache.size() == zx4_img.count())
                            {
                                thumb_cache.move(drag_source_frame, target_idx);
                            }
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
                    else
                    {
                        frame_strip->setCursor(Qt::ClosedHandCursor);
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
                auto * me = dynamic_cast<QMouseEvent *>(ev);
                SpriteWindow * target_sprite = me ? spriteWindowAtGlobalPos(me->globalPosition().toPoint()) : nullptr;

                const bool was_dragging = is_dragging_frame;
                const int init_frame = drag_initial_frame;
                const int src_frame = drag_source_frame;

                drag_initial_frame = -1;
                drag_source_frame = -1;
                is_dragging_frame = false;
                frame_strip->setCursor(Qt::PointingHandCursor);
                frame_strip->update();

                if (was_dragging && target_sprite && target_sprite != this)
                {
                    if (init_frame >= 0 && init_frame < zx4_img.count() && src_frame != init_frame)
                    {
                        zx4_img.move(src_frame, init_frame);
                        if (thumb_cache.size() == zx4_img.count())
                        {
                            thumb_cache.move(src_frame, init_frame);
                        }
                        if (src_imp_img.count() == zx4_img.count())
                        {
                            src_imp_img.move(src_frame, init_frame);
                        }
                        if (transformed_img.count() == zx4_img.count())
                        {
                            transformed_img.move(src_frame, init_frame);
                        }
                        selectFrame(init_frame);
                    }

                    if (init_frame >= 0 && init_frame < zx4_img.count())
                    {
                        int insert_idx = target_sprite->currentFrame() + 1;
                        if (me && target_sprite->frame_strip)
                        {
                            const QPoint strip_pt = target_sprite->frame_strip->mapFromGlobal(me->globalPosition().toPoint());
                            if (target_sprite->frame_strip->rect().contains(strip_pt))
                            {
                                insert_idx = target_sprite->frameIndexAtPos(strip_pt) + 1;
                            }
                        }
                        target_sprite->insertFrameAt(insert_idx, image(init_frame), "Copy frame");
                    }
                }
                else if (was_dragging && init_frame >= 0 && src_frame != init_frame)
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
        {
            QMouseEvent * me = dynamic_cast<QMouseEvent *>(ev);
            if (!me)
            {
                break;
            }
            if (ev->type() == QEvent::MouseButtonPress)
            {
                on_change = nullptr;
                last_draw_pos = QPoint(-1, -1);
                updateCanvasCursor();
            }
            else if (me->buttons() == Qt::NoButton)
            {
                break;
            }

            if (!ui->actionAnimate->isChecked())
            {
                int step = qMax(1, qMin((ui->canvas->width() - 1) / grid_x, (ui->canvas->height() - 1) / grid_y));
                QPoint pt = me->position().toPoint();
                int x = (pt.x() - 1) / step;
                int y = (pt.y() - 1) / step;
                if (ev->type() == QEvent::MouseMove && QPoint(x, y) == last_draw_pos)
                {
                    break;
                }
                last_draw_pos = QPoint(x, y);

                bool modified = false;
                if (me->buttons().testFlag(Qt::LeftButton))
                {
                    if (acts[ActionPencil] && acts[ActionPencil]->isChecked())
                    {
                        drawDot(x, y);
                        modified = true;
                        on_change = [this]()
                        {
                            storeToHistory("Pixel draw");
                            emit undoRedoMenuUpdateIsNeeded();
                        };
                    }
                    else if (acts[ActionBrush] && acts[ActionBrush]->isChecked())
                    {
                        drawBrush(x, y);
                        modified = true;
                        on_change = [this]()
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
                            modified = true;
                            on_change = [this]()
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
                            modified = true;
                            on_change = [this]()
                            {
                                storeToHistory("Sprite move");
                                emit undoRedoMenuUpdateIsNeeded();
                            };
                        }
                    }
                }
                if (me->buttons().testFlag(Qt::RightButton) ||
                    (me->buttons().testFlag(Qt::LeftButton) && acts[ActionEraser] && acts[ActionEraser]->isChecked()))
                {
                    eraseBrush(x, y);
                    modified = true;
                    on_change = [this]()
                    {
                        storeToHistory("Pixel erase");
                        emit undoRedoMenuUpdateIsNeeded();
                    };
                }
                if (modified)
                {
                    invalidateThumbnails(current_frame);
                    ui->canvas->update();
                    if (frame_strip)
                    {
                        frame_strip->update();
                    }
                    emit previewUpdateIsNeeded();
                }
            }
            break;
        }
        case QEvent::MouseButtonRelease:
            last_draw_pos = QPoint(-1, -1);
            updateCanvasCursor();
            if (on_change)
            {
                auto fn = on_change;
                on_change = nullptr;
                fn();
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
    if (zx4_img.isEmpty() || grid_x <= 0 || grid_y <= 0)
    {
        return;
    }
    const Zx4Image & frame = *zx4_img[current_frame];
    const QRgb c_rgb[4] = {
        clrs[0].rgba(),
        clrs[1].rgba(),
        clrs[2].rgba(),
        clrs[3].rgba()
    };

    QImage img(grid_x, grid_y, QImage::Format_ARGB32_Premultiplied);
    img.fill(0);
    for (int y = 0; y < grid_y; ++y)
    {
        QRgb * scan = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < grid_x; ++x)
        {
            if (frame.alpha_layer[x][y])
            {
                scan[x] = c_rgb[colorIndex(frame, x, y)];
            }
        }
    }
    p->drawImage(QRect(0, 0, grid_x * step, grid_y * step), img);
}

void SpriteWindow::drawGrid()
{
    if (zx4_img.isEmpty() || grid_x <= 0 || grid_y <= 0)
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
    p.fillRect(0, 0, side_x, side_y, QBrush(grid_color, Qt::Dense6Pattern));
    drawPixels(&p, step);

    if (step >= 2)
    {
        p.setPen(grid_color);
        QVector<QLine> lines;
        lines.reserve(grid_x + grid_y + 2);
        for (int y = 0; y <= grid_y; ++y)
        {
            lines.append(QLine(0, y * step, side_x, y * step));
        }
        for (int x = 0; x <= grid_x; ++x)
        {
            lines.append(QLine(x * step, 0, x * step, side_y));
        }
        p.drawLines(lines);
    }
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
    invalidateThumbnails(current_frame);
    updateFrameStrip();
    ui->canvas->update();
    emit previewUpdateIsNeeded();
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
    invalidateThumbnails(current_frame);
    updateFrameStrip();
    ui->canvas->update();
    emit previewUpdateIsNeeded();
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

    invalidateThumbnails();
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

    invalidateThumbnails();
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
