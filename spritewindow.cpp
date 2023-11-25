#include <QMouseEvent>
#include <QAction>
#include <QPainter>
#include <QFileInfo>
#include <QSpinBox>
#include <QTimer>

#include "spritewindow.h"
#include "ui_spritewindow.h"

SpriteWindow::SpriteWindow(const Zx4Palette & palettes, QWidget *parent) :
    QWidget(parent), Zx4BaseObject(palettes),
    ui(new Ui::SpriteWindow)
{
    ui->setupUi(this);
    setCanvas(ui->canvas);

    ui->toolBar->addWidget(new QLabel("Frame"));

    ui->frameBox->setVisible(true);
    ui->toolBar->addWidget(ui->frameBox);

    clrs << Qt::black << Qt::black << Qt::black << Qt::black;
    setFileName(QString());
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
    ui->frameBox->setMaximum(i.count());
}

void SpriteWindow::setImage(const QList<const Zx4Image *> & i)
{
    ui->actionAnimate->setChecked(false);
    Zx4BaseObject::setImage(i);
    ui->frameBox->setMaximum(i.count());
}

QSize SpriteWindow::sizeHint() const
{
    return QSize(640, 640);
}

void SpriteWindow::setAction(ActionEnum e, QAction * a)
{
    acts[e] = a;
}

void SpriteWindow::setColors(QList<QColor> _colors)
{
    if (_colors.count() != 4)
    {
        return;
    }
    clrs = _colors;
    ui->canvas->update();
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
        zx4_img[k]->resize(grid_x, grid_y);
    }
    ui->canvas->update();
}

void SpriteWindow::setCanvasSize(QSize s)
{
    grid_x = s.width();
    grid_y = s.height();
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

        stored_sprite = * zx4_img[i];
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
        else
        if (new_image_x < old_grid_x)
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

        stored_sprite = * zx4_img[i];
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
        else
        if (new_image_y < old_grid_y)
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

    ui->canvas->update();

}


bool SpriteWindow::eventFilter(QObject * o, QEvent * ev)
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
            on_change = nullptr;
            if (!ui->actionAnimate->isChecked())
            {
                QMouseEvent * me = dynamic_cast<QMouseEvent *>(ev);
                int step = qMin(ui->canvas->width() / grid_x, ui->canvas->height() / grid_y);

                QPoint pt = me->pos();
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
                    else
                    if (acts[ActionBrush] && acts[ActionBrush]->isChecked())
                    {
                        drawBrush(x, y);
                        on_change = [=]()
                        {
                            storeToHistory("Brush draw");
                            emit undoRedoMenuUpdateIsNeeded();
                        };
                    }
                    else
                    if (acts[ActionMove] && acts[ActionMove]->isChecked())
                    {
                        if (ev->type() == QEvent::MouseButtonPress)
                        {
                            stored_sprite = * zx4_img[current_frame];
                            old_grid_pos = QPoint(x, y);
                        }
                        else
                        {
                            if (old_grid_pos != QPoint(x, y))
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
                    }                    
                    QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);
                }
                if (me->buttons().testFlag(Qt::RightButton) || (me->buttons().testFlag(Qt::LeftButton) && acts[ActionEraser] && acts[ActionEraser]->isChecked()))
                {
                    eraseDot(x, y);
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
    int i;
    if (!img.color_layer0[x][y] && !img.color_layer1[x][y])
    {
        i = 0;
    }
    else
    if (!img.color_layer0[x][y] && img.color_layer1[x][y])
    {
        i = 1;
    }
    else
    if (img.color_layer0[x][y] && !img.color_layer1[x][y])
    {
        i = 2;
    }
    else
    {
        i = 3;
    }
    return i;
}

void SpriteWindow::drawPixels(QPainter * p, int step)
{
    for (int y = 0; y < grid_y; y++)
    {
        for (int x = 0; x < grid_x; x++)
        {
            if (zx4_img[current_frame]->alpha_layer[x][y])
            {
                p->fillRect(x * step, y * step, step, step, clrs[colorIndex(* zx4_img[current_frame], x, y)]);
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

    zx4_img[current_frame]->resize(grid_x, grid_y);

    int step = qMin((ui->canvas->width() - 1) / grid_x, (ui->canvas->height() - 1) / grid_y);
    int side_x = step * grid_x;
    int side_y = step * grid_y;
    QColor grid_color(55, 55, 55);
    QPainter p(ui->canvas);
    p.fillRect(ui->canvas->rect(), Qt::black);
    p.fillRect(ui->canvas->rect(), QBrush(grid_color, Qt::Dense6Pattern));

    drawPixels(& p, step);

    p.setPen(grid_color);
    for (int y = 0; y <= grid_y; y++)
    {
        p.drawLine(0, y * step, side_x, y * step);
    }
    for (int x = 0; x <= grid_x; x++)
    {
        p.drawLine(x * step, 0, x * step, side_y);
    }
    QMetaObject::invokeMethod(this, "previewUpdateIsNeeded", Qt::QueuedConnection);
}

void SpriteWindow::drawDot(int x, int y)
{
    if (x < 0 || y < 0 || x >= grid_x || y >= grid_y)
    {
        return;
    }

    if (current_color_index == 0)
    {
        zx4_img[current_frame]->color_layer0[x][y] = Zx4Image::BitOff;
        zx4_img[current_frame]->color_layer1[x][y] = Zx4Image::BitOff;
    }
    else
    if (current_color_index == 1)
    {
        zx4_img[current_frame]->color_layer0[x][y] = Zx4Image::BitOff;
        zx4_img[current_frame]->color_layer1[x][y] = Zx4Image::BitOn;
    }
    else
    if (current_color_index == 2)
    {
        zx4_img[current_frame]->color_layer0[x][y] = Zx4Image::BitOn;
        zx4_img[current_frame]->color_layer1[x][y] = Zx4Image::BitOff;
    }
    else
    {
        zx4_img[current_frame]->color_layer0[x][y] = Zx4Image::BitOn;
        zx4_img[current_frame]->color_layer1[x][y] = Zx4Image::BitOn;
    }
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
    drawDot(x - 1, y - 2);
    drawDot(x, y - 2);
    drawDot(x + 1, y - 2);

    drawDot(x - 2, y - 1);
    drawDot(x - 1, y - 1);
    drawDot(x, y - 1);
    drawDot(x + 1, y - 1);
    drawDot(x + 2, y - 1);

    drawDot(x - 2, y);
    drawDot(x - 1, y);
    drawDot(x, y);
    drawDot(x + 1, y);
    drawDot(x + 2, y);

    drawDot(x - 2, y + 1);
    drawDot(x - 1, y + 1);
    drawDot(x, y + 1);
    drawDot(x + 1, y + 1);
    drawDot(x + 2, y + 1);

    drawDot(x - 1, y + 2);
    drawDot(x, y + 2);
    drawDot(x + 1, y + 2);
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
    grid_x = qMax(img.width(), grid_x);
    grid_y = qMax(img.height(), grid_y);
    clearCanvas(QSize(grid_x, grid_y));

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
    ui->canvas->update();

#if 0
    QLabel * w = new QLabel();
    w->setScaledContents(true);
    w->setPixmap(QPixmap::fromImage(img));
    w->resize(4 * img.width(), 4 * img.height());
    w->show();
#endif
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
    ui->canvas->update();
    storeToHistory("Colors inversion");
    emit undoRedoMenuUpdateIsNeeded();
}

void SpriteWindow::on_actionAnimate_toggled(bool state)
{
    if (state)
    {
        animationFrame();
    }
}

void SpriteWindow::on_actionStop_triggered()
{
    ui->actionAnimate->setChecked(false);
    on_frameBox_valueChanged(ui->frameBox->minimum());
}

void SpriteWindow::animationFrame()
{
    if (ui->actionAnimate->isChecked())
    {
        ui->frameBox->setValue(ui->frameBox->value() == ui->frameBox->maximum() ? ui->frameBox->minimum() : ui->frameBox->value() + 1);
        QTimer::singleShot(200, this, SLOT(animationFrame()));
    }
}

void SpriteWindow::on_frameBox_valueChanged(int i)
{
    setCurrentFrame(i - 1);
    ui->canvas->update();
}

