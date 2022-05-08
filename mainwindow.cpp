#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lz4.h"
#include "lz4hc.h"

#include <QPainter>
#include <QResizeEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QProgressDialog>
#include <QMimeData>
#include <QImageReader>
#include <QImageWriter>

#include <omp.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->preview->installEventFilter(this);
    ui->sourceImage->installEventFilter(this);
    ui->mdiArea->installEventFilter(this);

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

    QMetaObject::invokeMethod(this, "selectPalette", Qt::QueuedConnection, Q_ARG(int, palettes.grayscale_index));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionQuit_triggered()
{
    close();
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

void MainWindow::on_palettes_itemClicked(QListWidgetItem *)
{
    on_palettes_itemSelectionChanged();
}

void MainWindow::on_actionNew_triggered()
{
    current_sprite = new SpriteWindow(palettes);
    importParsFromObjectToWidget(* current_sprite);
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

    if (current_sprite)
    {
        importParsFromWidgetToObject(* current_sprite);
        reimport(current_sprite);
    }
    else
    if (current_scene)
    {
        importParsFromWidgetToObject(* current_scene);
        reimport(current_scene);
    }
}


void MainWindow::on_actionInvert_colors_triggered()
{
    if (current_sprite)
    {
        current_sprite->invertColors();
    }
    if (current_scene)
    {
        current_scene->invertColors();
    }
}

void MainWindow::on_actionImport_triggered()
{
    QString path = current_sprite ? QFileInfo(current_sprite->fileName()).absolutePath() : QString();
    if (path.isEmpty())
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filter;
    QStringList filters = QStringList() << "Images (*.png *.apng *.jpg *.jpeg *.jfif *.gif *.bmp)" << "ZX4 Pixelizer sprites (*.zx4)";
    QString filename = QFileDialog::getOpenFileName(this, "Import file", path, filters.join(";;"), & filter);
    if (filename.isEmpty())
    {
        return;
    }

    if (filters.indexOf(filter) == 0)
    {
        QImageReader img_rdr(filename);

        QImage src_imp_image;
        QSize sz;
        int p_idx = -1;
        QList<QImage> list;
        while (!(src_imp_image = img_rdr.read()).isNull())
        {
            list << src_imp_image;
        }
        if (current_sprite)
        {
            sz = current_sprite->canvasSize();
            p_idx = current_sprite->paletteIndex();
        }
        importSprite(list, filename, sz, p_idx);
    }
    else
    if (filters.indexOf(filter) == 1)
    {
        importSpriteZx4Format(filename);
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

    QString filter;
    QStringList filters = QStringList() << "ZX4 Pixelizer sprites (*.zx4)" << "ZX Spectrum tape (*.tap)";
    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, filters.join(";;"), & filter);
    if (filters.indexOf(filter) == 0)
    {
        exportSpriteZx4Format(filename);
    }
    else
    if (filters.indexOf(filter) == 1)
    {
        Zx4ImageWithAttributes image(current_sprite->image(), current_sprite->paletteIndex());
        Zx4ImageWithAttributes scene(255, 192);
        for (int x = 0; x < scene.width(); x += image.width())
        {
            for (int y = 0; y < scene.height(); y += image.height())
            {
                scene.copy(image, QPoint(x, y));
            }
        }
        exportTape(scene, filename);
    }
}

void MainWindow::on_actionOpen_triggered()
{
    QString path;
    SpriteWindow * _current_sprite = current_sprite;
    if (_current_sprite)
    {
        path = QFileInfo(_current_sprite->fileName()).absolutePath();
    }
    if (path.isEmpty())
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString filename = QFileDialog::getOpenFileName(this, "Open file", path, "ZX4 Pixelizer images (*.zx4.png *.zx4.apng)");
    if (filename.isEmpty())
    {
        return;
    }

    QImageReader img_rdr(filename);

    QImage src_imp_image;
    QList<QImage> list;
    while (!(src_imp_image = img_rdr.read()).isNull())
    {
        list << src_imp_image;
    }

    openSprite(list, filename);
}

void MainWindow::on_actionSave_as_triggered()
{
    SpriteWindow * _current_sprite = current_sprite;
    if (!_current_sprite)
    {
        return;
    }

    QString path = QFileInfo(_current_sprite->fileName()).absolutePath();
    if (!path.isEmpty())
    {
        path += "/" + QFileInfo(_current_sprite->fileName()).baseName();
    }
    else
    {
        path = QDir::homePath() + "/Pictures/";
    }

    QString ext = (_current_sprite->framesCount() <= 1 ? "zx4.png" : "zx4.apng");
    QString filter;
    QStringList filters = QStringList() << "ZX4 Pixelizer images (*." + ext + ")";
    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, filters.join(";;"), & filter);

    if (filename.isEmpty())
    {
        return;
    }

    while (filename.endsWith(ext, Qt::CaseInsensitive))
    {
        filename = filename.remove(filename.count() - ext.length() - 1, ext.length() + 1);
    }
    filename += "." + ext;

    _current_sprite->setFileName(filename);
    ui->actionSave->trigger();
}

void MainWindow::on_actionSave_triggered()
{
    SpriteWindow * _current_sprite = current_sprite;
    if (!_current_sprite)
    {
        return;
    }

    if (_current_sprite->fileName().isEmpty())
    {
        ui->actionSave_as->trigger();
        return;
    }

    QVector<QRgb> color_table;
    foreach(QColor color, palettes.palettes[ui->palettes->currentRow()])
    {
        color_table << color.rgb();
    }
    color_table << 0; // alpha

    QImageWriter iw(_current_sprite->fileName(), _current_sprite->framesCount() <= 1 ? "png" : "apng");

    for (int k = 0; k < _current_sprite->framesCount(); k++)
    {
        QImage img(_current_sprite->canvasSize().width(), _current_sprite->canvasSize().height(), QImage::Format_Indexed8);
        img.setColorTable(color_table);
        for (int y = 0; y < _current_sprite->canvasSize().height(); y++)
        {
            for (int x = 0; x < _current_sprite->canvasSize().width(); x++)
            {
                if (!_current_sprite->image(k).alpha_layer[x][y])
                {
                    img.setPixel(x, y, 4);
                }
                else
                {
                    img.setPixel(x, y, _current_sprite->colorIndex(_current_sprite->image(k), x, y));
                }
            }
        }
        if (!iw.write(img))
        {
            QMessageBox::critical(this, "Error", "Error occured while file saving:\n" + iw.errorString());
            return;
        }
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
    importScene(QImage(filename), filename);
}

void MainWindow::on_actionExport_scene_triggered()
{
    if (!current_scene)
    {
        return;
    }

    Zx4Image scene = current_scene->image();
    auto scene_attr = current_scene->attr();

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
    }
    else
    if (filters.indexOf(filter) == 1)
    {
        exportTape(* dynamic_cast<const Zx4ImageWithAttributes *>(& current_scene->image()), filename);
    }
    else
    if (filters.indexOf(filter) == 0)
    {
        exportScreens(* dynamic_cast<const Zx4ImageWithAttributes *>(& current_scene->image()), filename);
    }
}


void MainWindow::on_actionAbout_ZX4_Pixelizer_triggered()
{
    QMessageBox::about(this, "ZX4 Pixelizer", QString("ZX4 Pixelizer\nImages & sprites manipulation tool\nfor ZX Spectrum 128K\n\nVersion %1\n\n© 2022\nZXLDR - ZX Lab of Dreams Reinvention").arg(APP_VERSION));
}


void MainWindow::on_mdiArea_subWindowActivated(QMdiSubWindow *msw)
{
    if (current_sprite)
    {
        importParsFromWidgetToObject(* current_sprite);
    }
    if (current_scene)
    {
        importParsFromWidgetToObject(* current_scene);
    }

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
        importParsFromObjectToWidget(* current_sprite);
    }
    else
    if (SceneWindow * sw = dynamic_cast<SceneWindow *>(msw->widget()))
    {
        current_scene = sw;
        importParsFromObjectToWidget(* current_scene);
    }
}

void MainWindow::selectPalette(int p_idx)
{
    ui->palettes->setCurrentRow(p_idx);

    ui->paletteIndex->blockSignals(true);
    ui->paletteIndex->setCurrentIndex(p_idx);
    ui->paletteIndex->blockSignals(false);

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

void MainWindow::on_paletteIndex_currentIndexChanged(int index)
{
    selectPalette(index);
}
