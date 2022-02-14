#include "mainwindow.h"
#include "ui_mainwindow.h"

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

    //    ui->palettes->setCurrentRow(327 - 1);
    ui->paletteIndex->setCurrentIndex(540 - 1);
    QMetaObject::invokeMethod(this, "on_paletteIndex_currentIndexChanged", Qt::QueuedConnection, Q_ARG(int, ui->paletteIndex->currentIndex()));
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
            break;
            default:
            break;
        }
    }
    return QMainWindow::eventFilter(o, ev);
}

void MainWindow::on_palettes_itemSelectionChanged()
{
    QList<QColor> list = palettes.palettes[ui->palettes->currentRow()];
    QPixmap px(QSize(32, 32));
    px.fill(list[0]);
    ui->actionColor_00->setIcon(px);
    px.fill(list[1]);
    ui->actionColor_01->setIcon(px);
    px.fill(list[2]);
    ui->actionColor_10->setIcon(px);
    px.fill(list[3]);
    ui->actionColor_11->setIcon(px);

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

    ui->paletteIndex->setCurrentIndex(ui->palettes->currentRow());
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

    ui->palettes->setCurrentRow(palette_number);

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
        ui->actionResize_canvas->trigger();
    }

    QString path = QFileInfo(current_sprite->fileName()).absolutePath();
    if (path.isEmpty())
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filename = QFileDialog::getOpenFileName(this, "Import file", path, "Images (*.png *.jpg *.jpeg *.jfif *.gif *.bmp)");
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
    current_sprite->setSourceImage(src_imp_image);
    ui->actionImportWizardApply->trigger();
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
        path += "/" + QFileInfo(current_sprite->fileName()).fileName() + ".zx4";
    }
    else
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, "ZX4 Pixelizer images (*.zx4)");
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
    QByteArray zx4(4 + 3 * layer_sz, 0);
    int offs = 0;
    zx4[offs++] = (z[1] & 0x07) | ((z[0] & 0x0F) << 3);
    zx4[offs++] = (z[3] & 0x07) | ((z[2] & 0x0F) << 3);
    zx4[offs++] = current_sprite->canvasSize().width();
    zx4[offs++] = current_sprite->canvasSize().height();

    for (int y = 0; y < current_sprite->canvasSize().height(); y++)
    {
        const QVector<char> & alpha = current_sprite->image().alpha_layer[y];
        const QVector<char> & line0 = current_sprite->image().color_layer0[y];
        const QVector<char> & line1 = current_sprite->image().color_layer1[y];
        for (int x = 0; x < (current_sprite->canvasSize().width() + 7) / 8; x++, offs++)
        {
            for (int bit = 0; bit < 8 && (8 * x) + bit < current_sprite->canvasSize().width(); bit++)
            {
                zx4[offs] =                 static_cast<char>(zx4[offs] | (static_cast<int>(alpha[(8 * x) + (7 - bit)]) << bit));
                zx4[offs + layer_sz] =      static_cast<char>(zx4[offs + layer_sz] | (static_cast<int>(line0[(8 * x) + (7 - bit)]) << bit));
                zx4[offs + 2 * layer_sz] =  static_cast<char>(zx4[offs + 2 * layer_sz] | (static_cast<int>(line1[(8 * x) + (7 - bit)]) << bit));
            }
        }
    }
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
    if (!current_sprite)
    {
        ui->actionNew->trigger();
        ui->actionResize_canvas->trigger();
    }

    QString path = QFileInfo(current_sprite->fileName()).absolutePath();
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
    current_sprite->setPaletteIndex(p_idx);
    ui->palettes->setCurrentRow(p_idx);

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
        path += "/" + QFileInfo(current_sprite->fileName()).fileName() + ".zx4.png";
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

#define GetY(y)         (((y & 0xc0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2))
#define GetXY(x, y)     (GetY(y) | (x & 0x1F))

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
        path += "/" + QFileInfo(current_scene->fileName()).fileName() + ".scr0";
    }
    else
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filter;
    QStringList filters = QStringList() << "ZX Spectrum screen (*.scr0)" << "PNG image (*.png)";
    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, filters.join(";;"), & filter);
    if (filename.isEmpty())
    {
        return;
    }

    if (filters.indexOf(filter) == 1)
    {        
        if (QFileInfo(filename).suffix().compare("png", Qt::CaseInsensitive) != 0)
        {
            filename = filename.replace("." + QFileInfo(filename).suffix(), ".png");
        }
        current_scene->renderedImage().convertToFormat(QImage::Format_RGB32).save(filename, "PNG");
        return;
    }

    if (QFileInfo(filename).suffix().compare("scr0", Qt::CaseInsensitive) != 0)
    {
        filename = filename.replace("." + QFileInfo(filename).suffix(), ".scr0");
    }

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
    QFile f0(filename), f1(filename.replace(".scr0", ".scr1", Qt::CaseInsensitive));
    f0.open(QFile::WriteOnly);
    f1.open(QFile::WriteOnly);
    if (f0.isOpen() && f1.isOpen())
    {
        f0.write(scr0);
        f1.write(scr1);
    }
    f0.close();
    f1.close();
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
        ui->palettes->setCurrentRow(current_sprite->paletteIndex());
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
}

void MainWindow::on_paletteIndex_currentIndexChanged(int index)
{
    ui->palettes->setCurrentRow(index);
}

