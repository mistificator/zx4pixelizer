#include <QMouseEvent>
#include <QAction>
#include <QPainter>
#include <QFileInfo>

#include "spritewindow.h"
#include "ui_spritewindow.h"

SpriteWindow::SpriteWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SpriteWindow)
{
    ui->setupUi(this);
    ui->canvas->installEventFilter(this);

    clrs << Qt::black << Qt::black << Qt::black << Qt::black;
}

SpriteWindow::~SpriteWindow()
{
    delete ui;
}

void SpriteWindow::setFileName(QString s)
{
    current_file_name = s;
    setWindowTitle(QFileInfo(current_file_name).fileName());
}

QString SpriteWindow::fileName() const
{
    return current_file_name;
}

QSize SpriteWindow::sizeHint() const
{
    return QSize(640, 640);
}

QLabel * SpriteWindow::canvas() const
{
    return ui->canvas;
}

const Zx4Image & SpriteWindow::image() const
{
    return img;
}

void SpriteWindow::setSourceImage(QImage i)
{
    src_imp_img = i;
}

const QImage & SpriteWindow::sourceImage() const
{
    return src_imp_img;
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

void SpriteWindow::setCurrentColorIndex(int _c)
{
    current_color_index = _c;
    ui->canvas->update();
}

int SpriteWindow::currentColorIndex() const
{
    return current_color_index;
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
    setFileName(QString());
    img = Zx4Image();
    img.resize(grid_x = s.width(), grid_y = s.height());
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
    int new_image_x = s.width();
    int new_image_y = s.height();

    stored_img = img;
    int old_grid_x = grid_x;
    if (new_image_x > old_grid_x)
    {
        grid_x = new_image_x;
    }
    img.resize(grid_x, grid_y);

    if (new_image_x > old_grid_x)
    {
        for (int y = 0; y < grid_y; y++)
        {
            for (int x = 0; x < grid_x; x++)
            {
                img.color_layer0[y][x] = stored_img.color_layer0[y][x * old_grid_x / grid_x];
                img.color_layer1[y][x] = stored_img.color_layer1[y][x * old_grid_x / grid_x];
                img.alpha_layer[y][x] = stored_img.alpha_layer[y][x * old_grid_x / grid_x];
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
                img.color_layer0[y][x] = stored_img.color_layer0[y][x * old_grid_x / new_image_x];
                img.color_layer1[y][x] = stored_img.color_layer1[y][x * old_grid_x / new_image_x];
                img.alpha_layer[y][x] = stored_img.alpha_layer[y][x * old_grid_x / new_image_x];
            }
            for (; x < grid_x; x++)
            {
                img.alpha_layer[y][x] = 0;
            }
        }
    }

    stored_img = img;
    int old_grid_y = grid_y;
    if (new_image_y > old_grid_y)
    {
        grid_y = new_image_y;
    }
    img.resize(grid_x, grid_y);

    if (new_image_y > old_grid_y)
    {
        for (int y = 0; y < grid_y; y++)
        {
            img.color_layer0[y] = stored_img.color_layer0[y * old_grid_y / grid_y];
            img.color_layer1[y] = stored_img.color_layer1[y * old_grid_y / grid_y];
            img.alpha_layer[y] = stored_img.alpha_layer[y * old_grid_y / grid_y];
        }
    }
    else
    if (new_image_y < old_grid_y)
    {
        int y = 0;
        for (; y < new_image_y; y++)
        {
            img.color_layer0[y] = stored_img.color_layer0[y * old_grid_y / new_image_y];
            img.color_layer1[y] = stored_img.color_layer1[y * old_grid_y / new_image_y];
            img.alpha_layer[y] = stored_img.alpha_layer[y * old_grid_y / new_image_y];
        }
        for (; y < grid_y; y++)
        {
            img.alpha_layer[y] = QVector<char>(grid_x);
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
                    }
                    else
                    if (acts[ActionBrush] && acts[ActionBrush]->isChecked())
                    {
                        drawBrush(x, y);
                    }
                    else
                    if (acts[ActionMove] && acts[ActionMove]->isChecked())
                    {
                        if (ev->type() == QEvent::MouseButtonPress)
                        {
                            stored_img = img;
                            old_grid_pos = QPoint(x, y);
                        }
                        else
                        {
                            if (old_grid_pos != QPoint(x, y))
                            {
                                int dx = old_grid_pos.x() - x;
                                int dy = old_grid_pos.y() - y;
                                moveImage(dx, dy);
                            }
                        }
                    }
                }
                if (me->buttons().testFlag(Qt::RightButton) || (me->buttons().testFlag(Qt::LeftButton) && acts[ActionEraser] && acts[ActionEraser]->isChecked()))
                {
                    eraseDot(x, y);
                }
                QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);

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
    if (!img.color_layer0[y][x] && !img.color_layer1[y][x])
    {
        i = 0;
    }
    else
    if (!img.color_layer0[y][x] && img.color_layer1[y][x])
    {
        i = 1;
    }
    else
    if (img.color_layer0[y][x] && !img.color_layer1[y][x])
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
            if (img.alpha_layer[y][x])
            {
                p->fillRect(x * step, y * step, step, step, clrs[colorIndex(img, x, y)]);
            }
        }
    }
}

void SpriteWindow::drawGrid()
{
    img.resize(grid_x, grid_y);

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
        img.color_layer0[y][x] = 0;
        img.color_layer1[y][x] = 0;
    }
    else
    if (current_color_index == 1)
    {
        img.color_layer0[y][x] = 0;
        img.color_layer1[y][x] = 1;
    }
    else
    if (current_color_index == 2)
    {
        img.color_layer0[y][x] = 1;
        img.color_layer1[y][x] = 0;
    }
    else
    {
        img.color_layer0[y][x] = 1;
        img.color_layer1[y][x] = 1;
    }
    img.alpha_layer[y][x] = 1;
}

void SpriteWindow::eraseDot(int x, int y)
{
    if (x < 0 || y < 0 || x >= grid_x || y >= grid_y)
    {
        return;
    }
    img.alpha_layer[y][x] = 0;
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
        if (my + dy >= 0 && my + dy < grid_y)
        {
            img.color_layer0[my] = stored_img.color_layer0[my + dy];
            img.color_layer1[my] = stored_img.color_layer1[my + dy];
            img.alpha_layer[my] = stored_img.alpha_layer[my + dy];
        }
        else
        {
            img.alpha_layer[my] = QVector<char>(grid_x);
        }
        if (dx > 0)
        {
            img.color_layer0[my] = img.color_layer0[my].mid(dx, grid_x - dx) + QVector<char>(dx);
            img.color_layer1[my] = img.color_layer1[my].mid(dx, grid_x - dx) + QVector<char>(dx);
            img.alpha_layer[my] = img.alpha_layer[my].mid(dx, grid_x - dx) + QVector<char>(dx);
        }
        else
        if (dx < 0)
        {
            img.color_layer0[my] = QVector<char>(-dx) + img.color_layer0[my].mid(0, grid_x + dx);
            img.color_layer1[my] = QVector<char>(-dx) + img.color_layer1[my].mid(0, grid_x + dx);
            img.alpha_layer[my] = QVector<char>(-dx) + img.alpha_layer[my].mid(0, grid_x + dx);
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
    if (img.isNull())
    {
        return;
    }
    for (int y = 0; y < grid_y; y++)
    {
        QVector<char> & line0 = img.color_layer0[y];
        QVector<char> & line1 = img.color_layer1[y];
        for (int x = 0; x < grid_x; x++)
        {
            line0[x] = line0[x] ? 0 : 1;
            line1[x] = line1[x] ? 0 : 1;
        }
    }
    ui->canvas->update();
}
