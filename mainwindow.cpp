#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QResizeEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->canvas->installEventFilter(this);
    ui->preview->installEventFilter(this);

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

    for (int i = 0; i < ui->baseColor->count(); i++)
    {
        QPixmap px(32, 32);
        px.fill(color_table[(ZxColorEnum)(i + 8)]);
        ui->baseColor->setItemIcon(i, px);
    }

    QActionGroup * ag = new QActionGroup(this);
    ag->setExclusive(true);
    ag->addAction(ui->actionPencil);
    ag->addAction(ui->actionBrush);
    ag->addAction(ui->actionMove);
    ag->addAction(ui->actionEraser);
    ui->pencilButton->setDefaultAction(ui->actionPencil);
    ui->brushButton->setDefaultAction(ui->actionBrush);
    ui->moveButton->setDefaultAction(ui->actionMove);
    ui->eraserButton->setDefaultAction(ui->actionEraser);

    ui->menuWindow->addAction(ui->dockPalette->toggleViewAction());
    ui->menuWindow->addAction(ui->dockDrawTools->toggleViewAction());
    ui->menuWindow->addAction(ui->dockPreview->toggleViewAction());

    on_baseColor_activated(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_baseColor_activated(int index)
{
    int row = qMax(ui->palettes->currentIndex().row(), 0);
    int col = qMax(ui->palettes->currentIndex().column(), 0);
    ui->palettes->clear();
    for (int i = 0; i < 8; i++)
    {
        for (int k = 0; k < 8; k++)
        {
            for (int m = 0; m < 8; m++)
            {
                for (int b1 = 0; b1 < 2; b1++)
                {
                    for (int b2 = 0; b2 < 2; b2++)
                    {
                        QColor c[4];
                        ZxColorEnum z[4];
                        c[0] = color_table[z[0] = (ZxColorEnum)(index + 8 * b1)];
                        c[1] = color_table[z[1] = (ZxColorEnum)(i + 8 * b1)];
                        c[2] = color_table[z[2] = (ZxColorEnum)(k + 8 * b2)];
                        c[3] = color_table[z[3] = (ZxColorEnum)(m + 8 * b2)];

                        QPixmap px(64, 64);
                        QPainter p(& px);
                        QColor m[4];
                        p.fillRect(0, 0, px.width() / 2, px.height() / 2,                               m[0] = QColor::fromRgb((c[0].red() + c[2].red()) / 2, (c[0].green() + c[2].green()) / 2, (c[0].blue() + c[2].blue()) / 2));
                        p.fillRect(px.width() / 2, 0, px.width() / 2, px.height() / 2,                  m[1] = QColor::fromRgb((c[0].red() + c[3].red()) / 2, (c[0].green() + c[3].green()) / 2, (c[0].blue() + c[3].blue()) / 2));
                        p.fillRect(0, px.height() / 2, px.width() / 2, px.height() / 2,                 m[2] = QColor::fromRgb((c[1].red() + c[2].red()) / 2, (c[1].green() + c[2].green()) / 2, (c[1].blue() + c[2].blue()) / 2));
                        p.fillRect(px.width() / 2, px.height() / 2, px.width() / 2, px.height() / 2,    m[3] = QColor::fromRgb((c[1].red() + c[3].red()) / 2, (c[1].green() + c[3].green()) / 2, (c[1].blue() + c[3].blue()) / 2));

                        QListWidgetItem * item = new QListWidgetItem();
                        item->setData(Qt::UserRole, QVariantList() << m[0] << m[1] << m[2] << m[3]);
                        item->setToolTip((QStringList() << color_name[z[0]] << color_name[z[1]] << color_name[z[2]] << color_name[z[3]]).join("\n"));
                        item->setIcon(px);
                        ui->palettes->addItem(item);
                    }
                }
            }
        }
    }
    ui->palettes->setCurrentIndex(ui->palettes->model()->index(row, col));
    on_palettes_itemSelectionChanged();
}


void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::drawPixels(QPainter * p, int step)
{
    QList<QColor> list = colors();
    for (int y = 0; y < grid_y; y++)
    {
        for (int x = 0; x < grid_x; x++)
        {
            if (image.alpha_layer[y][x])
            {
                QColor c;
                if (!image.color_layer0[y][x] && !image.color_layer1[y][x])
                {
                    c = list[0];
                }
                else
                if (!image.color_layer0[y][x] && image.color_layer1[y][x])
                {
                    c = list[1];
                }
                else
                if (image.color_layer0[y][x] && !image.color_layer1[y][x])
                {
                    c = list[2];
                }
                else
                {
                    c = list[3];
                }
                p->fillRect(x * step, y * step, step, step, c);
            }
        }
    }
}

void MainWindow::resizeGrid()
{
    image.color_layer0.resize(grid_y);
    image.color_layer1.resize(grid_y);
    image.alpha_layer.resize(grid_y);
    for (int i = 0; i < grid_y; i++)
    {
        image.color_layer0[i].resize(grid_x);
        image.color_layer1[i].resize(grid_x);
        image.alpha_layer[i].resize(grid_x);
    }
}

void MainWindow::drawGrid()
{    
    resizeGrid();

    int step = qMin((ui->canvas->width() - 1) / grid_x, (ui->canvas->height() - 1) / grid_y);
    int side_x = step * grid_x;
    int side_y = step * grid_y;
    QPainter p(ui->canvas);
    p.fillRect(ui->canvas->rect(), Qt::black);
    p.fillRect(ui->canvas->rect(), QBrush(Qt::gray, Qt::Dense6Pattern));

    drawPixels(& p, step);

    p.setPen(Qt::gray);
    for (int y = 0; y <= grid_y; y++)
    {
        p.drawLine(0, y * step, side_x, y * step);
    }
    for (int x = 0; x <= grid_x; x++)
    {
        p.drawLine(x * step, 0, x * step, side_y);
    }
    QMetaObject::invokeMethod(ui->preview, "update", Qt::QueuedConnection);
}

void MainWindow::drawDot(int x, int y)
{
    if (x < 0 || y < 0 || x >= grid_x || y >= grid_y)
    {
        return;
    }

    if (current_color == 0)
    {
        image.color_layer0[y][x] = 0;
        image.color_layer1[y][x] = 0;
    }
    else
    if (current_color == 1)
    {
        image.color_layer0[y][x] = 0;
        image.color_layer1[y][x] = 1;
    }
    else
    if (current_color == 2)
    {
        image.color_layer0[y][x] = 1;
        image.color_layer1[y][x] = 0;
    }
    else
    {
        image.color_layer0[y][x] = 1;
        image.color_layer1[y][x] = 1;
    }
    image.alpha_layer[y][x] = 1;
}

void MainWindow::eraseDot(int x, int y)
{
    if (x < 0 || y < 0 || x >= grid_x || y >= grid_y)
    {
        return;
    }
    image.alpha_layer[y][x] = 0;
}

void MainWindow::moveImage(int dx, int dy)
{
    for (int my = 0; my < grid_y; my++)
    {
        if (my + dy >= 0 && my + dy < grid_y)
        {
            image.color_layer0[my] = stored_image.color_layer0[my + dy];
            image.color_layer1[my] = stored_image.color_layer1[my + dy];
            image.alpha_layer[my] = stored_image.alpha_layer[my + dy];
        }
        else
        {
            image.alpha_layer[my] = QVector<char>(grid_x);
        }
        if (dx > 0)
        {
            image.color_layer0[my] = image.color_layer0[my].mid(dx, grid_x - dx) + QVector<char>(dx);
            image.color_layer1[my] = image.color_layer1[my].mid(dx, grid_x - dx) + QVector<char>(dx);
            image.alpha_layer[my] = image.alpha_layer[my].mid(dx, grid_x - dx) + QVector<char>(dx);
        }
        else
        if (dx < 0)
        {
            image.color_layer0[my] = QVector<char>(-dx) + image.color_layer0[my].mid(0, grid_x + dx);
            image.color_layer1[my] = QVector<char>(-dx) + image.color_layer1[my].mid(0, grid_x + dx);
            image.alpha_layer[my] = QVector<char>(-dx) + image.alpha_layer[my].mid(0, grid_x + dx);
        }
    }
}

bool MainWindow::eventFilter(QObject * o, QEvent * ev)
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
                    if (ui->actionPencil->isChecked())
                    {
                        drawDot(x, y);
                    }
                    else
                    if (ui->actionBrush->isChecked())
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
                    else
                    if (ui->actionMove->isChecked())
                    {
                        if (ev->type() == QEvent::MouseButtonPress)
                        {
                            stored_image = image;
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
                if (me->buttons().testFlag(Qt::RightButton) || (me->buttons().testFlag(Qt::LeftButton) && ui->actionEraser->isChecked()))
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
    else
    if (o == ui->preview)
    {
        switch (ev->type())
        {
            case QEvent::Paint:
            {               
                int step = qMin(ui->preview->width() / grid_x, ui->preview->height() / grid_y);
                int side_x = step * grid_x;
                int side_y = step * grid_y;

                QPainter p(ui->preview);
                p.fillRect(0, 0, side_x, side_y, Qt::black);
                drawPixels(& p, step);
            }
            break;
            default:
            break;
        }
    }
    return QMainWindow::eventFilter(o, ev);
}

QList<QColor> MainWindow::colors() const
{
    QList<QColor> list = QList<QColor>() << Qt::black << Qt::black << Qt::black << Qt::black;
    QListWidgetItem * item = ui->palettes->currentItem();
    if (!item)
    {
        return list;
    }
    QVariantList v = item->data(Qt::UserRole).toList();
    if (v.isEmpty() || v.count() != 4)
    {
        return list;
    }
    list[0] = v.takeFirst().value<QColor>();
    list[1] = v.takeFirst().value<QColor>();
    list[2] = v.takeFirst().value<QColor>();
    list[3] = v.takeFirst().value<QColor>();
    return list;
}

void MainWindow::on_palettes_itemSelectionChanged()
{
    QList<QColor> list = colors();
    QPixmap px(ui->color00->iconSize());
    px.fill(list[0]);
    ui->color00->setIcon(px);
    px.fill(list[1]);
    ui->color01->setIcon(px);
    px.fill(list[2]);
    ui->color10->setIcon(px);
    px.fill(list[3]);
    ui->color11->setIcon(px);

    updateColorArea();
    ui->canvas->update();
}


void MainWindow::on_color00_clicked()
{
    current_color = 0;
    updateColorArea();
}


void MainWindow::on_color01_clicked()
{
    current_color = 1;
    updateColorArea();
}


void MainWindow::on_color10_clicked()
{
    current_color = 2;
    updateColorArea();
}


void MainWindow::on_color11_clicked()
{
    current_color = 3;
    updateColorArea();
}

void MainWindow::updateColorArea()
{
    QPixmap px(64, 64);
    px.fill(colors()[current_color]);
    ui->currenColorArea->setPixmap(px);
}

void MainWindow::on_actionResize_canvas_triggered()
{
    bool ok = false;
    int new_grid_x = QInputDialog::getInt(this, "Width", "New canvas width", grid_x, 1, 255, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    if (!ok)
    {
        return;
    }
    int new_grid_y = QInputDialog::getInt(this, "Height", "New canvas height", grid_y, 1, 255, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    if (!ok)
    {
        return;
    }
    grid_x = new_grid_x;
    grid_y = new_grid_y;
    update();
}


void MainWindow::on_actionResize_image_triggered()
{
    bool ok = false;
    int new_image_x = QInputDialog::getInt(this, "Width", "New image width", grid_x, 1, 255, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    if (!ok)
    {
        return;
    }
    int new_image_y = QInputDialog::getInt(this, "Height", "New image height", grid_y, 1, 255, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    if (!ok)
    {
        return;
    }

    stored_image = image;
    int old_grid_x = grid_x;
    if (new_image_x > old_grid_x)
    {
        grid_x = new_image_x;
    }
    resizeGrid();

    if (new_image_x > old_grid_x)
    {
        for (int y = 0; y < grid_y; y++)
        {
            for (int x = 0; x < grid_x; x++)
            {
                image.color_layer0[y][x] = stored_image.color_layer0[y][x * old_grid_x / grid_x];
                image.color_layer1[y][x] = stored_image.color_layer1[y][x * old_grid_x / grid_x];
                image.alpha_layer[y][x] = stored_image.alpha_layer[y][x * old_grid_x / grid_x];
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
                image.color_layer0[y][x] = stored_image.color_layer0[y][x * old_grid_x / new_image_x];
                image.color_layer1[y][x] = stored_image.color_layer1[y][x * old_grid_x / new_image_x];
                image.alpha_layer[y][x] = stored_image.alpha_layer[y][x * old_grid_x / new_image_x];
            }
            for (; x < grid_x; x++)
            {
                image.alpha_layer[y][x] = 0;
            }
        }
    }

    stored_image = image;
    int old_grid_y = grid_y;
    if (new_image_y > old_grid_y)
    {
        grid_y = new_image_y;
    }
    resizeGrid();

    if (new_image_y > old_grid_y)
    {
        for (int y = 0; y < grid_y; y++)
        {
            image.color_layer0[y] = stored_image.color_layer0[y * old_grid_y / grid_y];
            image.color_layer1[y] = stored_image.color_layer1[y * old_grid_y / grid_y];
            image.alpha_layer[y] = stored_image.alpha_layer[y * old_grid_y / grid_y];
        }
    }
    else
    if (new_image_y < old_grid_y)
    {
        int y = 0;
        for (; y < new_image_y; y++)
        {
            image.color_layer0[y] = stored_image.color_layer0[y * old_grid_y / new_image_y];
            image.color_layer1[y] = stored_image.color_layer1[y * old_grid_y / new_image_y];
            image.alpha_layer[y] = stored_image.alpha_layer[y * old_grid_y / new_image_y];
        }
        for (; y < grid_y; y++)
        {
            image.alpha_layer[y] = QVector<char>(grid_x);
        }
    }

    update();
}


void MainWindow::on_actionImport_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, "Import file", QString(), "Images (*.png *.jpg *.jpeg *.jfif *.bmp)");
    if (filename.isEmpty())
    {
        return;
    }
    QImage src_img(filename);

    QImage img(src_img.scaled(grid_x, grid_y, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation).size(), src_img.format());
    img.fill(Qt::black);
    for (int y = 0; y < src_img.height(); y++)
    {
        for (int x = 0; x < src_img.width(); x++)
        {
            int ix = x * img.width() / src_img.width();
            int iy = y * img.height() / src_img.height();
            QColor old_px = img.pixelColor(ix, iy).toHsl();
            QColor new_px = src_img.pixelColor(x, y).toHsl();
            QColor px = new_px.hslSaturation() > old_px.hslSaturation() ? new_px : old_px;
            px.setHsl(
                        qMax<int>(qMin<int>(((px.hslHue() + 4) / 8) * 8, 255), 0),
                        qMax<int>(qMin<int>(((px.hslSaturation() + 4) / 8) * 8, 255), 0),
                        qMax<int>(qMin<int>(((px.lightness() + 4) / 8) * 8, 255), 0),
                        qMax<int>(qMin<int>(((px.alpha() + 4) / 8) * 8, 255), 0));
            img.setPixelColor(ix, iy, px.toRgb());
        }
    }

    QVector<QRgb> color_table;
    foreach (QColor color, colors())
    {
        color_table << color.rgb();
    }

    img = img.convertToFormat(QImage::Format_Indexed8, color_table);

    QLabel * w = new QLabel();
    w->setScaledContents(true);
    w->setPixmap(QPixmap::fromImage(img));
    w->resize(4 * img.width(), 4 * img.height());
    w->show();
}

