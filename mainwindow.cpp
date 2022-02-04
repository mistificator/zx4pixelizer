#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QResizeEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->canvas->installEventFilter(this);
    ui->preview->installEventFilter(this);
    ui->sourceImage->installEventFilter(this);

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

    ui->resizeImageButton->setDefaultAction(ui->actionResize_image);
    ui->resizeCanvasButton->setDefaultAction(ui->actionResize_canvas);
    ui->invertColorsButton->setDefaultAction(ui->actionInvert_colors);

    ui->importWizardApply->setDefaultAction(ui->actionImportWizardApply);

    ui->menuWindow->addAction(ui->dockPalette->toggleViewAction());
    ui->menuWindow->addAction(ui->dockDrawTools->toggleViewAction());
    ui->menuWindow->addAction(ui->dockPreview->toggleViewAction());
    ui->menuWindow->addAction(ui->dockSource->toggleViewAction());
    tabifyDockWidget(ui->dockPreview, ui->dockSource);
    tabifyDockWidget(ui->dockSource, ui->dockWizard);
    ui->dockPreview->raise();

    createPalettes();
    ui->palettes->setCurrentItem(ui->palettes->item(327 - 1));
    ui->palettes->setStyleSheet("QListWidget::item:selected { background: rgb(100, 255, 100); }");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createPalettes()
{
    int color_num = 0;
    for (int c0i = 0; c0i < 8; c0i++)
    {
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

                            QPixmap px(64, 64);
                            QPainter p(& px);
                            p.fillRect(0, 0, px.width() / 2, px.height() / 2,                               m[0]);
                            p.fillRect(px.width() / 2, 0, px.width() / 2, px.height() / 2,                  m[1]);
                            p.fillRect(0, px.height() / 2, px.width() / 2, px.height() / 2,                 m[2]);
                            p.fillRect(px.width() / 2, px.height() / 2, px.width() / 2, px.height() / 2,    m[3]);

                            QListWidgetItem * item = new QListWidgetItem();
                            palettes << (QList<QColor>() << m[0] << m[1] << m[2] << m[3]);
                            item->setData(Qt::UserRole, color_num);
                            item->setToolTip((QStringList() << QString("#%1").arg(color_num + 1) << color_name[z[0]] << color_name[z[1]] << color_name[z[2]] << color_name[z[3]]).join("\n"));
                            item->setIcon(px);
                            ui->palettes->addItem(item);

                            palettes_by_color[m[0]] << color_num;
                            palettes_by_color[m[1]] << color_num;
                            palettes_by_color[m[2]] << color_num;
                            palettes_by_color[m[3]] << color_num;

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

void MainWindow::showEvent(QShowEvent *)
{
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    ui->palettes->scrollToItem(ui->palettes->currentItem(), QAbstractItemView::PositionAtCenter);
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

int MainWindow::colorIndex(int x, int y) const
{
    int i;
    if (!image.color_layer0[y][x] && !image.color_layer1[y][x])
    {
        i = 0;
    }
    else
    if (!image.color_layer0[y][x] && image.color_layer1[y][x])
    {
        i = 1;
    }
    else
    if (image.color_layer0[y][x] && !image.color_layer1[y][x])
    {
        i = 2;
    }
    else
    {
        i = 3;
    }
    return i;
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
                p->fillRect(x * step, y * step, step, step, list[colorIndex(x, y)]);
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
    QMetaObject::invokeMethod(ui->preview, "update", Qt::QueuedConnection);
    QMetaObject::invokeMethod(ui->sourceImage, "update", Qt::QueuedConnection);
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

void MainWindow::drawBrush(int x, int y)
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
                        drawBrush(x, y);
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
            if (!image.isNull() && image.alpha_layer.count() == grid_y)
            {               
                int step = qMin(ui->preview->width() / grid_x, ui->preview->height() / grid_y);
                int side_x = step * grid_x;
                int side_y = step * grid_y;

                QList<QColor> c = colors();
                QImage pattern(4 * step, 4 * step, QImage::Format_RGB32);
                QPainter pp(& pattern);
                QList<int> idx = QList<int>() << 0 << 3 << 2 << 1 <<
                                                 1 << 0 << 3 << 2 <<
                                                 2 << 1 << 0 << 3 <<
                                                 3 << 2 << 1 << 0;
                for (int i = 0; i < idx.count(); i++)
                {
                    pp.fillRect((i % 4) * step, (i / 4) * step, step, step, c[idx[i]]);
                }
                QPainter p(ui->preview);
                p.fillRect(0, 0, side_x, side_y, pattern);
                drawPixels(& p, step);
            }
            break;
            default:
            break;
        }
    }
    else
    if (o == ui->sourceImage)
    {
        switch (ev->type())
        {
            case QEvent::Paint:
            if (!src_imp_image.isNull())
            {
                int step = qMin(ui->sourceImage->width() / grid_x, ui->sourceImage->height() / grid_y);

                int side_x = step * grid_x;
                int side_y = step * grid_y;
                if (src_imp_image.width() < src_imp_image.height())
                {
                    side_x = src_imp_image.width() * side_x / src_imp_image.height();
                }
                else
                {
                    side_y = src_imp_image.height() * side_y / src_imp_image.width();
                }
                QPainter p(ui->sourceImage);
                p.drawImage((step * grid_x - side_x) / 2, (step * grid_y - side_y) / 2, src_imp_image.scaled(side_x, side_y));
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
    return palettes.at(item->data(Qt::UserRole).toInt());
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
    int new_grid_y = QInputDialog::getInt(this, "Height", "New canvas height", grid_y, 1, 192, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
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
    int new_image_y = QInputDialog::getInt(this, "Height", "New image height", grid_y, 1, 192, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
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

QImage MainWindow::scaleImage(QImage img, int w, int h) const
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
            QColor old_px = out.pixelColor(ix, iy).toHsl();
            QColor new_px = img.pixelColor(x, y).toHsl();
            QColor px = new_px.hslSaturation() > old_px.hslSaturation() ? new_px : old_px;
#if 0
            px.setHsl(
                        qMax<int>(qMin<int>(((px.hslHue() + 4) / 8) * 8, 255), 0),
                        qMax<int>(qMin<int>(((px.hslSaturation() + 4) / 8) * 8, 255), 0),
                        qMax<int>(qMin<int>(((px.lightness() + 4) / 8) * 8, 255), 0),
                        new_px.alpha() > old_px.alpha() ? new_px.alpha() : old_px.alpha());
#else
            px.setHsl(
                        px.hslHue(),
                        px.hslSaturation(),
                        px.lightness(),
                        new_px.alpha() > old_px.alpha() ? new_px.alpha() : old_px.alpha());
#endif
            out.setPixelColor(ix, iy, px.toRgb());
        }
    }
    return out;
}

template <> inline bool qMapLessThanKey<QColor>(const QColor &key1, const QColor &key2)
{
    return key1.rgb() < key2.rgb();
}

template <> inline uint qHash<QColor>(const QColor &t, uint seed)
    noexcept(noexcept(qHash(t.rgba())))
{
    return qHash(t.rgba()) ^ seed;
}

template <> inline bool operator< <QColor, QColor>(const QPair<QColor, QColor> &p1, const QPair<QColor, QColor> &p2)
{
    return p1.first.rgba() < p2.first.rgba() || (!(p2.first.rgba() < p1.first.rgba()) && p1.second.rgba() < p2.second.rgba());
}

template <>
class qLess<QColor>
{
public:
    inline bool operator()(const QColor &t1, const QColor &t2) const
    {
        return (t1.rgba() < t2.rgba());
    }
};

QMultiMap<int, QColor> MainWindow::colorDistribution(QImage img, bool use_alpha) const
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

bool MainWindow::alikeColors(QColor c1, QColor c2, int hue_precision) const
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
    if (qAbs(c1.lightness() - c2.lightness()) > 48)
    {
        return false;
    }
    if (qAbs(c1.hslSaturation() - c2.hslSaturation()) > 32)
    {
        return false;
    }
    return true;
}

QMap<QColor, int> MainWindow::colorDistribution4(QImage img, int scale_x, int scale_y) const
{
    QMultiMap<int, QColor> color_distrib2 = colorDistribution(scaleImage(img, qMin(img.width(), scale_x), qMin(img.height(), scale_y)));

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

QMultiMap<double, int> MainWindow::selectPalette(QMap<QColor, int> color_distrib4) const
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

QMultiMap<double, int> MainWindow::selectPalette(QImage img, int scale_x, int scale_y) const
{
    return selectPalette(colorDistribution4(img, scale_x, scale_y));
}

QImage MainWindow::importImage(QImage img, int palette_number, bool use_alpha)
{
    ui->palettes->setCurrentItem(ui->palettes->item(palette_number));

    img = scaleImage(img, grid_x, grid_y);

    QVector<QRgb> color_table;
    foreach (QColor color, colors())
    {
        color_table << color.rgb();
    }

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

void MainWindow::drawImage(QImage img)
{
    QVector<QRgb> color_table = img.colorTable();

    grid_x = qMax(img.width(), grid_x);
    grid_y = qMax(img.height(), grid_y);
    ui->actionNew->trigger();

    int x_offs = (grid_x - img.width()) / 2;
    int y_offs = (grid_y - img.height()) / 2;
    for (int y = 0; y < img.height(); y++)
    {
        for (int x = 0; x < img.width(); x++)
        {
            int idx = color_table.indexOf(img.pixel(x, y));
            if (idx >= 0 && idx < 4)
            {
                current_color = idx;
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

void MainWindow::on_palettes_itemClicked(QListWidgetItem *)
{
    on_palettes_itemSelectionChanged();
}

void MainWindow::on_actionNew_triggered()
{
    current_file_name.clear();
    image = Zx4Image();
    resizeGrid();
    update();
}

void MainWindow::on_actionImportWizardApply_triggered()
{
    int p_idx = ui->palettes->currentRow();
    if (ui->wizardPaletteBox->currentIndex() == 0)
    {
        QMultiMap<double, int> dist = selectPalette(src_imp_image);
        ui->paletteNumberBox->setRange(1, dist.count() + 1);
        p_idx = * (dist.begin() + (ui->paletteNumberBox->value() - 1));
    }
    drawImage(importImage(src_imp_image,  p_idx, ui->alphaBox->isChecked()));
}


void MainWindow::on_actionInvert_colors_triggered()
{
    if (image.isNull())
    {
        return;
    }
    for (int y = 0; y < grid_y; y++)
    {
        QVector<char> & line0 = image.color_layer0[y];
        QVector<char> & line1 = image.color_layer1[y];
        for (int x = 0; x < grid_x; x++)
        {
            line0[x] = line0[x] ? 0 : 1;
            line1[x] = line1[x] ? 0 : 1;
        }
    }
    update();
}

void MainWindow::on_actionImport_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, "Import file", QDir::homePath() + "/Pictures/", "Images (*.png *.jpg *.jpeg *.jfif *.bmp)");
    if (filename.isEmpty())
    {
        return;
    }
    src_imp_image = QImage(filename).convertToFormat(QImage::Format_ARGB32);
    if (src_imp_image.isNull())
    {
        QMessageBox::critical(this, "Error", QString("Error occured while opening \"%1\"").arg(filename));
        return;
    }

    ui->actionImportWizardApply->trigger();
}

void MainWindow::on_actionOpen_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open file", QDir::homePath() + "/Pictures/", "Images (*.zx4.png)");
    if (filename.isEmpty())
    {
        return;
    }

    src_imp_image = QImage(filename);
    if (src_imp_image.isNull())
    {
        QMessageBox::critical(this, "Error", QString("Error occured while opening \"%1\"").arg(filename));
        return;
    }

    if (src_imp_image.width() > 255 || src_imp_image.height() > 192)
    {
        src_imp_image = QImage();
        QMessageBox::critical(this, "Error", "Wrong image size");
        return;
    }

    if (src_imp_image.colorTable().count() != 5)
    {
        src_imp_image = QImage();
        QMessageBox::critical(this, "Error", "Wrong image color table");
        return;
    }

    current_file_name = filename;

    grid_x = qMin(src_imp_image.width(), 255);
    grid_y = qMin(src_imp_image.height(), 192);
    ui->actionNew->trigger();
    drawImage(src_imp_image);

    src_imp_image = src_imp_image.convertToFormat(QImage::Format_ARGB32);
    update();
}


void MainWindow::on_actionSave_as_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save file", QDir::homePath() + "/Pictures/", "Images (*.zx4.png)");
    if (filename.isEmpty())
    {
        return;
    }
    if (QFileInfo(filename).suffix().isEmpty())
    {
        filename += ".zx4.png";
    }

    current_file_name = filename;

    ui->actionSave->trigger();
}

void MainWindow::on_actionSave_triggered()
{
    if (current_file_name.isEmpty())
    {
        ui->actionSave_as->trigger();
        return;
    }

    QImage img(grid_x, grid_y, QImage::Format_Indexed8);
    QVector<QRgb> color_table;
    foreach(QColor color, colors())
    {
        color_table << color.rgb();
    }
    color_table << 0; // alpha
    img.setColorTable(color_table);
    for (int y = 0; y < grid_y; y++)
    {
        for (int x = 0; x < grid_x; x++)
        {
            if (!image.alpha_layer[y][x])
            {
                img.setPixel(x, y, 4);
            }
            else
            {
                img.setPixel(x, y, colorIndex(x, y));
            }
        }
    }
    if (!img.save(current_file_name, "png"))
    {
        QMessageBox::critical(this, "Error", "Error occured while file saving");
    }
}

void MainWindow::on_wizardPaletteBox_currentIndexChanged(int)
{
    ui->paletteNumberBox->setEnabled(ui->wizardPaletteBox->currentIndex() == 0);
}

void MainWindow::on_actionImport_scene_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, "Import file", QDir::homePath() + "/Pictures/", "Images (*.png *.jpg *.jpeg *.jfif *.bmp)");
    if (filename.isEmpty())
    {
        return;
    }
    QImage img = QImage(filename).convertToFormat(QImage::Format_RGB32);
    if (img.isNull())
    {
        QMessageBox::critical(this, "Error", QString("Error occured while opening \"%1\"").arg(filename));
        return;
    }

    img = scaleImage(img, qMin(((img.width() + 7) / 8) * 8, 256), qMin(((img.height() + 7) / 8) * 8, 192)).convertToFormat(QImage::Format_Indexed8);
//    img = img.scaled(qMin(((img.width() + 7) / 8) * 8, 256), qMin(((img.height() + 7) / 8) * 8, 192), Qt::KeepAspectRatio, Qt::FastTransformation).convertToFormat(QImage::Format_Indexed8);

    QImage out(img.width(), img.height(), QImage::Format_RGB32);
    QPainter p(& out);

    for (int y = 0; y < img.height(); y += 8)
    {
        for (int x = 0; x < img.width(); x += 8)
        {
            QImage small_quad = img.copy(QRect(x, y, 8, 8));
            auto small_dist = colorDistribution(small_quad, false);

            int p_idx = -1;
            if (!small_dist.isEmpty())
            {
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

                do
                {
                    QList<QColor> colors = dist4.values();
                    qSort(colors); // indexes "palettes_by_color" sorted by color

                    int hue_precision = 5;
                    do
                    {
                        if (colors.count() == 1)
                        {
                            for (auto i = palettes_by_color.begin(); i != palettes_by_color.end(); ++i)
                            {
                                if (alikeColors(i.key(), colors.first(), hue_precision))
                                {
                                    p_idx = i.value().at(i.value().count() * 0.1);
                                    break;
                                }
                            }
                        }
                        else
                        if (colors.count() == 2)
                        {
                            for (auto i = palettes_by_2colors.begin(); i != palettes_by_2colors.end(); ++i)
                            {
                                if (alikeColors(i.key().first, colors.first(), hue_precision) && alikeColors(i.key().second, colors.last(), hue_precision))
                                {
                                    p_idx = i.value().at(i.value().count() * 0.1);
                                    break;
                                }
                            }
                        }
                        else
                        if (colors.count() == 3)
                        {
                            for (auto i = palettes_by_3colors.begin(); i != palettes_by_3colors.end() && p_idx == -1; ++i)
                            {
                                if (alikeColors(i.key().first, colors.at(0), hue_precision) && alikeColors(i.key().second, colors.at(1), hue_precision))
                                {
                                    for (auto k = i.value().begin(); k != i.value().end(); ++k)
                                    {
                                        if (alikeColors(k.key(), colors.at(2)))
                                        {
                                            p_idx = k.value().at(k.value().count() * 0.1);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else
                        if (colors.count() == 4)
                        {
                            for (auto i = palettes_by_4colors.begin(); i != palettes_by_4colors.end() && p_idx == -1; ++i)
                            {
                                if (alikeColors(i.key().first, colors.at(0), hue_precision) && alikeColors(i.key().second, colors.at(1), hue_precision))
                                {
                                    for (auto k = i.value().begin(); k != i.value().end(); ++k)
                                    {
                                        if (alikeColors(k.key().first, colors.at(2), hue_precision) && alikeColors(k.key().second, colors.at(3), hue_precision))
                                        {
                                            p_idx = k.value().at(k.value().count() * 0.1);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        hue_precision += 5;
                    }
                    while (p_idx == -1 && hue_precision < 100 / (colors.count()));

                    dist4.erase(dist4.begin());
                }
                while (p_idx == -1 && !dist4.isEmpty());
            }

            if (p_idx != -1)
            {
                QVector<QRgb> color_table;
                foreach(QColor c, palettes[p_idx])
                {
                    color_table << c.rgb();
                }
                p.drawImage(QRect(x, y, 8, 8), small_quad.convertToFormat(QImage::Format_RGB32).convertToFormat(QImage::Format_Indexed8, color_table));
            }
        }
    }

    QLabel * w = new QLabel();
    w->setScaledContents(true);
    w->setPixmap(QPixmap::fromImage(out));
    w->resize(4 * out.width(), 4 * out.height());
    w->show();
}

