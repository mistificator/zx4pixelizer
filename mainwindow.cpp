#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lz4.h"

#include <QPainter>
#include <QResizeEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QProgressDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->preview->installEventFilter(this);
    ui->sourceImage->installEventFilter(this);

    QActionGroup * ag = new QActionGroup(this);
    ag->setExclusive(true);
    ag->addAction(ui->actionPencil);
    ag->addAction(ui->actionBrush);
    ag->addAction(ui->actionMove);
    ag->addAction(ui->actionEraser);

    ag = new QActionGroup(this);
    ag->setExclusive(true);
    ag->addAction(ui->actionColor_00);
    ag->addAction(ui->actionColor_01);
    ag->addAction(ui->actionColor_10);
    ag->addAction(ui->actionColor_11);

    ui->importWizardApply->setDefaultAction(ui->actionImportWizardApply);

    ui->menuWindow->addAction(ui->dockPalette->toggleViewAction());
    ui->menuWindow->addAction(ui->dockPreview->toggleViewAction());
    ui->menuWindow->addAction(ui->dockSource->toggleViewAction());
    tabifyDockWidget(ui->dockPreview, ui->dockSource);
    tabifyDockWidget(ui->dockSource, ui->dockWizard);
    ui->dockPreview->raise();

    createPalettes();
    ui->palettes->setStyleSheet("QListWidget::item:selected { background: rgb(100, 255, 100); }");

    ui->actionNew->trigger();

//    selectPalette(291 - 1);
    QMetaObject::invokeMethod(this, "selectPalette", Qt::QueuedConnection, Q_ARG(int, 291 - 1));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createPalettes()
{
    palettes.init();

    for (int p_idx = 0; p_idx < palettes.palettes.count(); p_idx++)
    {
        qApp->processEvents();

        QList<QColor> m = palettes.palettes[p_idx];
        QPixmap px(64, 64);
        QPainter p(& px);
        p.fillRect(0, 0, px.width() / 2, px.height() / 2,                               m[0]);
        p.fillRect(px.width() / 2, 0, px.width() / 2, px.height() / 2,                  m[1]);
        p.fillRect(0, px.height() / 2, px.width() / 2, px.height() / 2,                 m[2]);
        p.fillRect(px.width() / 2, px.height() / 2, px.width() / 2, px.height() / 2,    m[3]);

        QList<ZxColorEnum> z = palettes.color_palette_table[p_idx];
        QListWidgetItem * item = new QListWidgetItem();
        item->setData(Qt::UserRole, p_idx);
        item->setToolTip((QStringList() << QString("#%1").arg(p_idx + 1) << palettes.color_name[z[0]] << palettes.color_name[z[1]] << palettes.color_name[z[2]] << palettes.color_name[z[3]]).join("\n"));
        item->setIcon(px);
        ui->palettes->addItem(item);
        ui->paletteIndex->addItem(px, QString::number(p_idx + 1));
    }
    ui->paletteIndex->lineEdit()->setValidator(new QIntValidator(1, palettes.palettes.count()));
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

bool MainWindow::eventFilter(QObject * o, QEvent * ev)
{
    if (o == ui->preview)
    {
        switch (ev->type())
        {
            case QEvent::Paint:
            if (current_sprite && !current_sprite->image().isNull() && current_sprite->image().alpha_layer.count() == current_sprite->canvasSize().height())
            {               
                int step = qMin(ui->preview->width() / current_sprite->canvasSize().width(), ui->preview->height() / current_sprite->canvasSize().height());
                int side_x = step * current_sprite->canvasSize().width();
                int side_y = step * current_sprite->canvasSize().height();

                QList<QColor> c = current_sprite->colors();
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
                current_sprite->drawPixels(& p, step);
            }
            else
            if (current_scene && !current_scene->renderedImage().isNull())
            {
                QPainter p(ui->preview);
                p.drawImage(0, 0, current_scene->renderedImage().scaled(ui->preview->size(), Qt::KeepAspectRatio));
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
            if (current_sprite && !current_sprite->sourceImage().isNull())
            {
                int step = qMin(ui->sourceImage->width() / current_sprite->canvasSize().width(), ui->sourceImage->height() / current_sprite->canvasSize().height());

                int side_x = step * current_sprite->canvasSize().width();
                int side_y = step * current_sprite->canvasSize().height();
                if (current_sprite->sourceImage().width() < current_sprite->sourceImage().height())
                {
                    side_x = current_sprite->sourceImage().width() * side_x / current_sprite->sourceImage().height();
                }
                else
                {
                    side_y = current_sprite->sourceImage().height() * side_y / current_sprite->sourceImage().width();
                }
                QPainter p(ui->sourceImage);
                p.drawImage((step * current_sprite->canvasSize().width() - side_x) / 2, (step * current_sprite->canvasSize().height() - side_y) / 2, current_sprite->sourceImage().scaled(side_x, side_y));
            }
            else
            if (current_scene && !current_scene->sourceImage().isNull())
            {
                QPainter p(ui->sourceImage);
                p.drawImage(0, 0, current_scene->sourceImage().scaled(ui->sourceImage->size(), Qt::KeepAspectRatio));
            }
            break;
            default:
            break;
        }
    }
    return QMainWindow::eventFilter(o, ev);
}

void MainWindow::on_palettes_itemSelectionChanged()
{
    selectPalette(ui->palettes->currentRow());
}

void MainWindow::on_actionColor_00_toggled(bool)
{
    if (current_sprite)
    {
        current_sprite->setCurrentColorIndex(0);
    }
    if (current_scene)
    {
        current_scene->setCurrentColorIndex(0);
    }
}

void MainWindow::on_actionColor_01_toggled(bool)
{
    if (current_sprite)
    {
        current_sprite->setCurrentColorIndex(1);
    }
    if (current_scene)
    {
        current_scene->setCurrentColorIndex(1);
    }
}

void MainWindow::on_actionColor_10_toggled(bool)
{
    if (current_sprite)
    {
        current_sprite->setCurrentColorIndex(2);
    }
    if (current_scene)
    {
        current_scene->setCurrentColorIndex(2);
    }
}

void MainWindow::on_actionColor_11_toggled(bool)
{
    if (current_sprite)
    {
        current_sprite->setCurrentColorIndex(3);
    }
    if (current_scene)
    {
        current_scene->setCurrentColorIndex(3);
    }
}

void MainWindow::on_actionResize_canvas_triggered()
{
    if (!current_sprite)
    {
        return;
    }

    bool ok = false;
    int new_grid_x = QInputDialog::getInt(this, "Width", "New canvas width", current_sprite->canvasSize().width(), 1, 255, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    if (!ok)
    {
        return;
    }
    int new_grid_y = QInputDialog::getInt(this, "Height", "New canvas height", current_sprite->canvasSize().height(), 1, 192, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    if (!ok)
    {
        return;
    }
    current_sprite->setCanvasSize(QSize(new_grid_x, new_grid_y));
}


void MainWindow::on_actionResize_image_triggered()
{
    if (!current_sprite)
    {
        return;
    }

    bool ok = false;
    int new_image_x = QInputDialog::getInt(this, "Width", "New image width", current_sprite->canvasSize().width(), 1, 255, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    if (!ok)
    {
        return;
    }
    int new_image_y = QInputDialog::getInt(this, "Height", "New image height", current_sprite->canvasSize().height(), 1, 192, 1, & ok, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    if (!ok)
    {
        return;
    }

    current_sprite->resizeImage(QSize(new_image_x, new_image_y));
}

QImage MainWindow::importImage(QImage img, int palette_number, bool use_alpha)
{
    if (!current_sprite)
    {
        return QImage();
    }

    selectPalette(palette_number);

    img = Zx4Processing::scaleImage(img, current_sprite->canvasSize().width(), current_sprite->canvasSize().height());

    QVector<QRgb> color_table;
    foreach (QColor color, palettes.palettes[palette_number])
    {
        color_table << color.rgb();
    }

    return Zx4Processing::importImage(img, color_table, use_alpha);
}

void MainWindow::on_palettes_itemClicked(QListWidgetItem *)
{
    on_palettes_itemSelectionChanged();
}

void MainWindow::on_actionNew_triggered()
{
    current_sprite = new SpriteWindow(palettes);
    current_sprite->setAction(SpriteWindow::ActionPencil, ui->actionPencil);
    current_sprite->setAction(SpriteWindow::ActionBrush, ui->actionBrush);
    current_sprite->setAction(SpriteWindow::ActionMove, ui->actionMove);
    current_sprite->setAction(SpriteWindow::ActionEraser, ui->actionEraser);
    QMdiSubWindow * msw = ui->mdiArea->addSubWindow(current_sprite);
    msw->show();

    current_sprite->clearCanvas(QSize(64, 64));
    connect(current_sprite, SIGNAL(previewUpdateIsNeeded()), ui->preview, SLOT(update()));
    connect(current_sprite, SIGNAL(previewUpdateIsNeeded()), ui->sourceImage, SLOT(update()));
}

void MainWindow::on_actionImportWizardApply_triggered()
{
    ui->paletteNumberBox->setToolTip(QString());
    if (!current_sprite)
    {
        return;
    }
    int p_idx = ui->palettes->currentRow();
    if (ui->wizardPaletteBox->currentIndex() == 0)
    {
        QMultiMap<double, int> dist = palettes.selectPalette(current_sprite->sourceImage());
        ui->paletteNumberBox->setRange(1, dist.count());
        ui->paletteNumberBox->setToolTip(QString("%1..%2").arg(1).arg(dist.count()));
        p_idx = * (dist.begin() + (ui->paletteNumberBox->value() - 1));
    }
    current_sprite->setPaletteIndex(p_idx);
    current_sprite->drawImage(importImage(current_sprite->sourceImage().convertToFormat(QImage::Format_ARGB32),  p_idx, ui->alphaBox->isChecked()));
}


void MainWindow::on_actionInvert_colors_triggered()
{
    if (current_sprite)
    {
        current_sprite->invertColors();
    }
}

void MainWindow::on_actionImport_triggered()
{
    if (!current_sprite)
    {
        ui->actionNew->trigger();
    }

    QString path = QFileInfo(current_sprite->fileName()).absolutePath();
    if (path.isEmpty())
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filter;
    QStringList filters = QStringList() << "Images (*.png *.jpg *.jpeg *.jfif *.gif *.bmp)" << "ZX4 Pixelizer sprites (*.zx4)";
    QString filename = QFileDialog::getOpenFileName(this, "Import file", path, filters.join(";;"), & filter);
    if (filename.isEmpty())
    {
        return;
    }

    if (filters.indexOf(filter) == 0)
    {
        ui->actionResize_canvas->trigger();

        QImage src_imp_image = QImage(filename);
        if (src_imp_image.isNull())
        {
            QMessageBox::critical(this, "Error", QString("Error occured while opening \"%1\"").arg(filename));
            return;
        }
        current_sprite->setSourceImage(src_imp_image);
        ui->actionImportWizardApply->trigger();
    }
    else
    if (filters.indexOf(filter) == 1)
    {
        QByteArray zx4;
        QFile f(filename);
        f.open(QFile::ReadOnly);
        if (f.isOpen())
        {
            zx4 = f.readAll();
        }
        f.close();
        if (zx4.isEmpty())
        {
            QMessageBox::critical(this, "Error", QString("Error occured while opening \"%1\"").arg(filename));
            return;
        }
        QList<ZxColorEnum> z;
        z << static_cast<ZxColorEnum>((zx4[0] & 0x78) >> 3);
        z << static_cast<ZxColorEnum>((zx4[0] & 0x07) | ((zx4[0] & 0x40) >> 3));
        z << static_cast<ZxColorEnum>((zx4[1] & 0x78) >> 3);
        z << static_cast<ZxColorEnum>((zx4[1] & 0x07) | ((zx4[1] & 0x40) >> 3));
        int p_idx = palettes.color_palette_table.key(z, -1);
        if (p_idx == -1)
        {
            QMessageBox::critical(this, "Error", QString("Error occured while applying palette for \"%1\"").arg(filename));
            return;
        }
        selectPalette(p_idx);
        current_sprite->setPaletteIndex(p_idx);
        current_sprite->setCanvasSize(QSize(zx4[2], zx4[3]));

        QByteArray zx4_lz4 = zx4.mid(4);
        zx4.resize(3 * current_sprite->canvasSize().width() * current_sprite->canvasSize().height());

        int isize = zx4_lz4.count();
        int osize = ::LZ4_uncompress_unknownOutputSize(zx4_lz4.constData(), zx4.data(), & isize, zx4.count());
        if (osize <= 0)
        {
            QMessageBox::critical(this, "Error", QString("Error occured while decompressing \"%1\"").arg(filename));
            return;
        }
        zx4.resize(osize);
        int alpha_bit = 0;
        for (int x = 0; x < (current_sprite->canvasSize().width() + 7) / 8; x++)
        {
            for (int y = 0; y < current_sprite->canvasSize().height(); y++)
            {
                QVector<char> & alpha = const_cast<Zx4Image &>(current_sprite->image()).alpha_layer[y];
                for (int bit = 0; bit < 8 && (8 * x) + bit < current_sprite->canvasSize().width(); bit++)
                {
                    alpha[(8 * x) + (7 - bit)] = (zx4[alpha_bit / 8] >> (alpha_bit % 8)) & 0x01;
                    ++alpha_bit;
                }
            }
        }
        zx4 = zx4.mid((alpha_bit + 7) / 8);
        int color_bit = 0;
        QByteArray zx4_color0 = zx4.left(zx4.count() / 2), zx4_color1 = zx4.mid(zx4.count() / 2);
        for (int x = 0; x < (current_sprite->canvasSize().width() + 7) / 8; x++)
        {
            for (int y = 0; y < current_sprite->canvasSize().height(); y++)
            {
                const QVector<char> & alpha = current_sprite->image().alpha_layer[y];
                QVector<char> & line0 = const_cast<Zx4Image &>(current_sprite->image()).color_layer0[y];
                QVector<char> & line1 = const_cast<Zx4Image &>(current_sprite->image()).color_layer1[y];
                for (int bit = 0; bit < 8 && (8 * x) + bit < current_sprite->canvasSize().width(); bit++)
                {
                    char alpha_value = alpha[(8 * x) + (7 - bit)];
                    if (alpha_value)
                    {
                        line0[(8 * x) + (7 - bit)] = (zx4_color0[color_bit / 8] >> (color_bit % 8)) & 0x01;
                        line1[(8 * x) + (7 - bit)] = (zx4_color1[color_bit / 8] >> (color_bit % 8)) & 0x01;
                        ++color_bit;
                    }
                }
            }
        }
        current_sprite->update();
    }
}

void MainWindow::on_actionExport_triggered()
{
    if (!current_sprite)
    {
        return;
    }

    QString path = QFileInfo(current_sprite->fileName()).absolutePath();
    if (!path.isEmpty())
    {
        path += "/" + QFileInfo(current_sprite->fileName()).baseName();
    }
    else
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, "ZX4 Pixelizer sprites (*.zx4)");
    if (filename.isEmpty())
    {
        return;
    }
    if (QFileInfo(filename).suffix().isEmpty())
    {
        filename += ".zx4";
    }

    int p_idx = ui->palettes->currentRow();
    QList<ZxColorEnum> z = palettes.color_palette_table[p_idx];

    const int layer_sz = ((current_sprite->canvasSize().width() * current_sprite->canvasSize().height() + 7) / 8);
    QByteArray zx4;
    zx4 += static_cast<char>(z[1] & 0x07) | ((z[0] & 0x0F) << 3);
    zx4 += static_cast<char>(z[3] & 0x07) | ((z[2] & 0x0F) << 3);
    zx4 += static_cast<char>(current_sprite->canvasSize().width());
    zx4 += static_cast<char>(current_sprite->canvasSize().height());

    int alpha_bit = 0, color_bit = 0;
    QByteArray zx4_alpha(layer_sz, 0), zx4_color0(layer_sz, 0), zx4_color1(layer_sz, 0);
    for (int x = 0; x < (current_sprite->canvasSize().width() + 7) / 8; x++)
    {
        for (int y = 0; y < current_sprite->canvasSize().height(); y++)
        {
            const QVector<char> & alpha = current_sprite->image().alpha_layer[y];
            const QVector<char> & line0 = current_sprite->image().color_layer0[y];
            const QVector<char> & line1 = current_sprite->image().color_layer1[y];
            for (int bit = 0; bit < 8 && (8 * x) + bit < current_sprite->canvasSize().width(); bit++)
            {
                char alpha_value = alpha[(8 * x) + (7 - bit)];
                zx4_alpha[alpha_bit / 8] =         static_cast<char>(zx4_alpha[alpha_bit / 8] | (static_cast<int>(alpha_value) << (alpha_bit % 8)));
                if (alpha_value)
                {
                    zx4_color0[color_bit / 8] =    static_cast<char>(zx4_color0[color_bit / 8] | (static_cast<int>(line0[(8 * x) + (7 - bit)]) << (color_bit % 8)));
                    zx4_color1[color_bit / 8] =    static_cast<char>(zx4_color1[color_bit / 8] | (static_cast<int>(line1[(8 * x) + (7 - bit)]) << (color_bit % 8)));
                    ++color_bit;
                }
                ++alpha_bit;
            }
        }
    }
    zx4_alpha.resize((alpha_bit + 7) / 8);
    zx4_color0.resize((color_bit + 7) / 8);
    zx4_color1.resize((color_bit + 7) / 8);
    const int zx4_lz4max_count = ::LZ4_compressBound(zx4_alpha.count() + zx4_color0.count() + zx4_color1.count());
    QByteArray zx4_data = zx4_alpha + zx4_color0 + zx4_color1, zx4_lz4(zx4_lz4max_count, 0);
    const int zx4_count = ::LZ4_compress(zx4_data.constData(), zx4_lz4.data(), zx4_data.count());
    zx4_lz4.resize(zx4_count);
    zx4 += zx4_lz4;
    qDebug() << "sprite deflated from" << 4 + 3 * layer_sz << "to" << zx4.count() << ", total ratio" << 100.0 * zx4.count() / (3 * layer_sz + 4) << "%";
    QFile f(filename);
    f.open(QFile::WriteOnly);
    if (f.isOpen())
    {
        f.write(zx4);
    }
    f.close();

}

void MainWindow::on_actionOpen_triggered()
{
    QString path;
    if (current_sprite)
    {
        path = QFileInfo(current_sprite->fileName()).absolutePath();
    }
    if (path.isEmpty())
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filename = QFileDialog::getOpenFileName(this, "Open file", path, "ZX4 Pixelizer images (*.zx4.png)");
    if (filename.isEmpty())
    {
        return;
    }

    QImage src_imp_image = QImage(filename);
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
    QVector<QRgb> colors = src_imp_image.colorTable().mid(0, 4);
    qSort(colors);
    int p_idx = ui->palettes->currentRow();
    if (palettes.palettes_by_4colors.contains(QPair<QColor, QColor>(colors[0], colors[1])) && palettes.palettes_by_4colors[QPair<QColor, QColor>(colors[0], colors[1])].contains(QPair<QColor, QColor>(colors[2], colors[3])))
    {
        QList<int> list = palettes.palettes_by_4colors[QPair<QColor, QColor>(colors[0], colors[1])][QPair<QColor, QColor>(colors[2], colors[3])];
        if (!list.isEmpty())
        {
            p_idx = list.first();
        }
    }
    selectPalette(p_idx);

    if (!current_sprite)
    {
        ui->actionNew->trigger();
    }

    current_sprite->setPaletteIndex(p_idx);
    current_sprite->setFileName(filename);
    current_sprite->clearCanvas(QSize(qMin(src_imp_image.width(), 255), qMin(src_imp_image.height(), 192)));
    current_sprite->setSourceImage(src_imp_image);
    current_sprite->drawImage(src_imp_image);
}


void MainWindow::on_actionSave_as_triggered()
{
    if (!current_sprite)
    {
        return;
    }

    QString path = QFileInfo(current_sprite->fileName()).absolutePath();
    if (!path.isEmpty())
    {
        path += "/" + QFileInfo(current_sprite->fileName()).baseName();
    }
    else
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, "ZX4 Pixelizer images (*.zx4.png)");
    if (filename.isEmpty())
    {
        return;
    }
    if (QFileInfo(filename).suffix().isEmpty())
    {
        filename += ".zx4.png";
    }

    current_sprite->setFileName(filename);
    ui->actionSave->trigger();
}

void MainWindow::on_actionSave_triggered()
{
    if (!current_sprite)
    {
        return;
    }

    if (current_sprite->fileName().isEmpty())
    {
        ui->actionSave_as->trigger();
        return;
    }

    QImage img(current_sprite->canvasSize().width(), current_sprite->canvasSize().height(), QImage::Format_Indexed8);
    QVector<QRgb> color_table;
    foreach(QColor color, palettes.palettes[ui->palettes->currentRow()])
    {
        color_table << color.rgb();
    }
    color_table << 0; // alpha
    img.setColorTable(color_table);
    for (int y = 0; y < current_sprite->canvasSize().height(); y++)
    {
        for (int x = 0; x < current_sprite->canvasSize().width(); x++)
        {
            if (!current_sprite->image().alpha_layer[y][x])
            {
                img.setPixel(x, y, 4);
            }
            else
            {
                img.setPixel(x, y, current_sprite->colorIndex(current_sprite->image(), x, y));
            }
        }
    }
    if (!img.save(current_sprite->fileName(), "png"))
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
    QString path = current_scene ? QFileInfo(current_scene->fileName()).absolutePath() : QString();
    if (path.isEmpty())
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filename = QFileDialog::getOpenFileName(this, "Import file", path, "Images (*.png *.jpg *.jpeg *.jfif *.gif *.bmp)");
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

    QProgressDialog pd(this, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    pd.setWindowIcon(QPixmap(":/sonic.zx4.png"));
    pd.setWindowTitle("ZX4 Pixelizer");
    pd.setLabelText("Importing scene");
    pd.setRange(0, 0);
    pd.setCancelButton(0);
    pd.show();
    qApp->processEvents();

    if (current_scene == 0)
    {
        current_scene = new SceneWindow(palettes);
        ui->mdiArea->addSubWindow(current_scene);
        current_scene->show();
        connect(current_scene, SIGNAL(charSelected(int)), this, SLOT(selectPalette(int)), Qt::QueuedConnection);
        connect(current_scene, SIGNAL(previewUpdateIsNeeded()), ui->preview, SLOT(update()));
        connect(current_scene, SIGNAL(previewUpdateIsNeeded()), ui->sourceImage, SLOT(update()));
    }
    current_scene->setFileName(filename);

//    img = scaleImage(img, qMin(((img.width() + 7) / 8) * 8, 256), qMin(((img.height() + 7) / 8) * 8, 192));
    img = img.scaled(qMin(((img.width() + 7) / 8) * 8, 256), qMin(((img.height() + 7) / 8) * 8, 192), Qt::KeepAspectRatio);
    current_scene->setSourceImage(img);

    QImage img8 = img.convertToFormat(QImage::Format_Indexed8);

#if 0
    auto img8_dist = colorDistribution(img8, false);
    qDebug() << img8_dist.count() << img8_dist;

    auto img_dist = colorDistribution(img, false);
    qDebug() << img_dist.count() << img_dist;
#endif

    Zx4Image scene;
    scene.resize(((img.width() + 7) / 8) * 8, ((img.height() + 7) / 8) * 8);
    QMap<int, QMap<int, int>> scene_attr;

    QImage out(img.width(), img.height(), QImage::Format_RGB32);
    QPainter p(& out);

    for (int y = 0; y < img.height(); y += 8)
    {
        for (int x = 0; x < img.width(); x += 8)
        {
            QImage small_quad = img8.copy(QRect(x, y, 8, 8));
            int p_idx = current_scene->selectPalette(small_quad);
            if (p_idx != -1)
            {
                current_scene->importChar(QPoint(x / 8, y / 8), p_idx, small_quad);
            }
        }
    }
    current_scene->canvas()->update();

    foreach(QMdiSubWindow * msw, ui->mdiArea->subWindowList())
    {
        if (msw->widget() == current_scene)
        {
            msw->resize(out.width() * 4, out.height() * 4);
            break;
        }
    }

    pd.hide();

#if 0
    QLabel * w = new QLabel();
    w->addAction(ui->actionExport_scene);
    w->setContextMenuPolicy(Qt::ActionsContextMenu);
    w->setScaledContents(true);
    w->setPixmap(QPixmap::fromImage(out));
    w->resize(4 * out.width(), 4 * out.height());
    w->show();
#endif
}

void MainWindow::on_actionExport_scene_triggered()
{
    if (!current_scene)
    {
        return;
    }

    Zx4Image scene = current_scene->image();
    QMap<int, QMap<int, int>> scene_attr = current_scene->attr();

    if (scene.isNull() || scene_attr.isEmpty())
    {
        return;
    }

    QString path = QFileInfo(current_scene->fileName()).absolutePath();
    if (!path.isEmpty())
    {
        path += "/" + QFileInfo(current_scene->fileName()).baseName();
    }
    else
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filter;
    QStringList filters = QStringList() << "ZX Spectrum screen (*.scr0)" << "ZX Spectrum tape (*.tap)" << "PNG image (*.png)";
    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, filters.join(";;"), & filter);
    if (filename.isEmpty())
    {
        return;
    }

    if (filters.indexOf(filter) == 2)
    {        
        if (QFileInfo(filename).suffix().compare("png", Qt::CaseInsensitive) != 0)
        {
            filename = filename.replace("." + QFileInfo(filename).suffix(), ".png");
        }
        current_scene->renderedImage().convertToFormat(QImage::Format_RGB32).save(filename, "PNG");
        return;
    }
    else
    if (filters.indexOf(filter) == 1)
    {
        if (QFileInfo(filename).suffix().compare("tap", Qt::CaseInsensitive) != 0)
        {
            filename = filename.replace("." + QFileInfo(filename).suffix(), ".tap");
        }
        QFile f(filename);
        f.open(QFile::WriteOnly);

        QFile ggf(":/GIGAVIEW.TAP");
        ggf.open(QFile::ReadOnly);
        f.write(ggf.readAll());
        ggf.close();

        auto screens = current_scene->makeScreens();
        int offset = 0xC0;
        foreach (QByteArray scr, QList<QByteArray>() << screens.first << screens.second)
        {
            unsigned short sz = scr.count();

            QByteArray h;
            h += (char)0x13;
            h += (char)0x00;
            h += (char)0x00;
            h += (char)0x03;
            h += "SCREEN    ";
            h += ((char *)& sz)[0];
            h += ((char *)& sz)[1];
            h += (char)0x00;
            h += (char)offset;
            h += (char)0x00;
            h += (char)0x00;

            char checksum = h[2];
            for (int i = 3; i < h.count(); i++)
            {
                checksum ^= h[i];
            }
            h += (char)checksum;
            offset += 0x1B;

            f.write(h);

            sz += 2;

            QByteArray d;
            d += ((char *)& sz)[0];
            d += ((char *)& sz)[1];
            d += (char)0xFF;
            d += scr;
            checksum = d[2];
            for (int i = 3; i < d.count(); i++)
            {
                checksum ^= d[i];
            }
            d += (char)checksum;

            f.write(d);
        }

        f.close();
        return;
    }
    else
    if (filters.indexOf(filter) == 0)
    {
        if (QFileInfo(filename).suffix().compare("scr0", Qt::CaseInsensitive) != 0)
        {
            filename = filename.replace("." + QFileInfo(filename).suffix(), ".scr0");
        }

        auto screens = current_scene->makeScreens();

        QFile f0(filename), f1(filename.replace(".scr0", ".scr1", Qt::CaseInsensitive));
        f0.open(QFile::WriteOnly);
        f1.open(QFile::WriteOnly);
        if (f0.isOpen() && f1.isOpen())
        {
            f0.write(screens.first);
            f1.write(screens.second);
        }
        f0.close();
        f1.close();
    }
}


void MainWindow::on_actionAbout_ZX4_Pixelizer_triggered()
{
    QMessageBox::about(this, "ZX4 Pixelizer", QString("ZX4 Pixelizer\nImages & sprites manipulation tool\nfor ZX Spectrum 128K\n\nVersion %1\n© 2022 ZXLDR Custom").arg(APP_VERSION));
}


void MainWindow::on_mdiArea_subWindowActivated(QMdiSubWindow *msw)
{
    current_sprite = 0;
    current_scene = 0;
    if (msw == 0)
    {
        return;
    }
    if (SpriteWindow * sw = dynamic_cast<SpriteWindow *>(msw->widget()))
    {
        current_sprite = sw;
        selectPalette(current_sprite->paletteIndex());
        switch (current_sprite->currentColorIndex())
        {
        case 0:
            ui->actionColor_00->setChecked(true);
            break;
        case 1:
            ui->actionColor_01->setChecked(true);
            break;
        case 2:
            ui->actionColor_10->setChecked(true);
            break;
        default:
            ui->actionColor_11->setChecked(true);
            break;
        }
    }
    else
    if (SceneWindow * sw = dynamic_cast<SceneWindow *>(msw->widget()))
    {
        current_scene = sw;
    }
}

void MainWindow::selectPalette(int p_idx)
{
    ui->palettes->setCurrentRow(p_idx);
    ui->paletteIndex->setCurrentIndex(p_idx);

    QList<QColor> list = palettes.palettes[p_idx];
    QPixmap px(QSize(32, 32));
    px.fill(list[0]);
    ui->actionColor_00->setIcon(px);
    ui->actionColor_00->setToolTip(palettes.color_name[palettes.color_palette_table[p_idx][0]] + " + " + palettes.color_name[palettes.color_palette_table[p_idx][2]]);
    px.fill(list[1]);
    ui->actionColor_01->setIcon(px);
    ui->actionColor_01->setToolTip(palettes.color_name[palettes.color_palette_table[p_idx][0]] + " + " + palettes.color_name[palettes.color_palette_table[p_idx][3]]);
    px.fill(list[2]);
    ui->actionColor_10->setIcon(px);
    ui->actionColor_10->setToolTip(palettes.color_name[palettes.color_palette_table[p_idx][1]] + " + " + palettes.color_name[palettes.color_palette_table[p_idx][2]]);
    px.fill(list[3]);
    ui->actionColor_11->setIcon(px);
    ui->actionColor_11->setToolTip(palettes.color_name[palettes.color_palette_table[p_idx][1]] + " + " + palettes.color_name[palettes.color_palette_table[p_idx][3]]);

    if (ui->mdiArea->activeSubWindow())
    {
        if (current_sprite && ui->mdiArea->activeSubWindow()->widget() == current_sprite)
        {
            current_sprite->setPaletteIndex(ui->palettes->currentRow());
            current_sprite->setColors(list);
        }
        if (current_scene && ui->mdiArea->activeSubWindow()->widget() == current_scene)
        {
            current_scene->setAttr(ui->palettes->currentRow());
            current_scene->canvas()->update();
        }
    }
}


