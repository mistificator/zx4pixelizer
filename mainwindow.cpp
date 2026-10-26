#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QActionGroup>
#include <QPainter>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QProgressDialog>
#include <QMimeData>
#include <QImageReader>
#include <QImageWriter>
#include <QMenu>
#include <QSettings>
#include <QSignalBlocker>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->preview->installEventFilter(this);
    ui->preview->setCursor(Qt::SplitHCursor);
    ui->mdiArea->installEventFilter(this);

    qApp->installEventFilter(this);

    QActionGroup * ag = new QActionGroup(this);
    ag->setExclusive(true);
    ag->addAction(ui->actionPencil);
    ag->addAction(ui->actionBrush);
    ag->addAction(ui->actionFill);
    ag->addAction(ui->actionMove);
    ag->addAction(ui->actionEraser);
    connect(ag, &QActionGroup::triggered, this, [this](QAction *)
            {
                if (current_sprite)
                {
                    current_sprite->updateCanvasCursor();
                }
            });

    auto setupToolSizePopup = [this](QAction * toolAction, int & targetSize)
    {
        QToolButton * btn = qobject_cast<QToolButton *>(ui->drawToolBar->widgetForAction(toolAction));
        if (!btn)
        {
            return;
        }
        btn->setPopupMode(QToolButton::MenuButtonPopup);
        QMenu * menu = new QMenu(btn);
        QActionGroup * sizeGroup = new QActionGroup(menu);
        sizeGroup->setExclusive(true);
        for (int sz : { 1, 3, 5, 7 })
        {
            QAction * act = menu->addAction(QString::number(sz));
            act->setCheckable(true);
            act->setChecked(sz == targetSize);
            sizeGroup->addAction(act);
            connect(act, &QAction::triggered, this, [this, toolAction, &targetSize, sz]()
                    {
                        targetSize = sz;
                        toolAction->setChecked(true);
                        if (current_sprite)
                        {
                            current_sprite->setBrushSize(current_brush_size);
                            current_sprite->setEraserSize(current_eraser_size);
                        }
                    });
        }
        btn->setMenu(menu);
    };

    setupToolSizePopup(ui->actionBrush, current_brush_size);
    setupToolSizePopup(ui->actionEraser, current_eraser_size);

    ag = new QActionGroup(this);
    ag->setExclusive(true);
    ag->addAction(ui->actionColor_00);
    ag->addAction(ui->actionColor_01);
    ag->addAction(ui->actionColor_10);
    ag->addAction(ui->actionColor_11);

    ui->importWizardApply->setDefaultAction(ui->actionImportWizardApply);

    QLabel * presetLabel = new QLabel(" Preset ", this);
    toolbarPresetBox = new QComboBox(this);
    toolbarPresetBox->setEditable(false);
    toolbarPresetBox->setMinimumWidth(150);

    toolbarDitherBox = new QCheckBox("Dither (pixelize)", this);
    toolbarDitherBox->setToolTip(ui->ditherBox->toolTip());

    toolbarReimportBtn = new QToolButton(this);
    toolbarReimportBtn->setDefaultAction(ui->actionImportWizardApply);

    ui->importToolBar->addWidget(presetLabel);
    ui->importToolBar->addWidget(toolbarPresetBox);
    ui->importToolBar->addSeparator();
    ui->importToolBar->addWidget(toolbarDitherBox);
    ui->importToolBar->addSeparator();
    ui->importToolBar->addWidget(toolbarReimportBtn);

    presetMenuGroup = new QActionGroup(this);
    presetMenuGroup->setExclusive(true);

    connect(toolbarPresetBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index)
            {
                if (index >= 0 && ui->presetBox->currentIndex() != index)
                {
                    ui->presetBox->setCurrentIndex(index);
                }
            });

    connect(ui->alphaBox, &QCheckBox::toggled, this, &MainWindow::syncAlphaState);
    connect(ui->ditherBox, &QCheckBox::toggled, this, &MainWindow::syncDitherState);
    connect(toolbarDitherBox, &QCheckBox::toggled, this, &MainWindow::syncDitherState);
    connect(ui->actionDither, &QAction::toggled, this, &MainWindow::syncDitherState);

    ui->menuWindow->addAction(ui->dockPalette->toggleViewAction());
    ui->menuWindow->addAction(ui->dockPreview->toggleViewAction());
    ui->menuWindow->addAction(ui->dockWizard->toggleViewAction());

    tabifyDockWidget(ui->dockPreview, ui->dockPalette);
    ui->dockPreview->raise();

    createPalettes();

    QMetaObject::invokeMethod(this, "selectPalette", Qt::QueuedConnection, Q_ARG(int, palettes.grayscale_index));

    QDir().mkpath(Zx4ImportPars::defaultPresetPath());
    auto ensurePreset = [](const QString &name, Zx4ImportPars::PassMethod pass, Zx4Palette::SelectPaletteMethod pick,
                           double hue, double sat, double light)
    {
        QString filePath = Zx4ImportPars::defaultPresetPath() + "/" + name;
        bool needWrite = !QFileInfo::exists(filePath);
        if (!needWrite)
        {
            QFile f(filePath);
            Zx4ImportPars test;
            if (!f.open(QFile::ReadOnly) || !test.deserialize(f.readAll()))
            {
                needWrite = true;
            }
        }
        if (needWrite)
        {
            Zx4ImportPars p;
            p.multipass = pass;
            p.color_pick = pick;
            p.hue_shift = hue;
            p.saturation_shift = sat;
            p.lightness_shift = light;
            QFile f(filePath);
            if (f.open(QFile::WriteOnly))
            {
                f.write(p.serialize());
                f.close();
            }
        }
    };

    ensurePreset("Default",             Zx4ImportPars::SinglePass,     Zx4Palette::SelectPaletteHSL,        0.00, 0.00,  0.00);
    ensurePreset("Dark Sharp",          Zx4ImportPars::Multipass7x7x3, Zx4Palette::SelectPaletteCIEDE2000,  0.00, 0.30, -0.15);
    ensurePreset("Acid ZX Contrast",    Zx4ImportPars::Multipass5x5x2, Zx4Palette::SelectPaletteCIEDE2000,  0.00, 0.55, -0.25);
    ensurePreset("Neon Gigascreen",     Zx4ImportPars::Multipass7x7x3, Zx4Palette::SelectPaletteCIEDE2000, -0.03, 0.45, -0.20);
    ensurePreset("Clean Pixel Art",     Zx4ImportPars::Multipass3x3x1, Zx4Palette::SelectPaletteCIE94,      0.00, 0.35, -0.15);
    ensurePreset("Ultra Sharp Poster",  Zx4ImportPars::Multipass5x5x2, Zx4Palette::SelectPaletteCIEDE2000,  0.00, 0.60, -0.40);

    syncPresetsUI("Default");
    on_presetBox_currentIndexChanged(ui->presetBox->currentIndex());
    updateToolbarWidgetsHeight();
    loadSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

QString MainWindow::configFilePath()
{
    return QDir::toNativeSeparators(QDir::homePath() + "/." + qApp->applicationName() + "/config.ini");
}

void MainWindow::saveSettings() const
{
    QDir().mkpath(QFileInfo(configFilePath()).absolutePath());
    QSettings settings(configFilePath(), QSettings::IniFormat);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/windowState", saveState());
    settings.setValue("Settings/alpha", ui->alphaBox->isChecked());
    settings.setValue("Settings/dithering", ui->ditherBox->isChecked());
}

void MainWindow::loadSettings()
{
    QSettings settings(configFilePath(), QSettings::IniFormat);
    if (settings.contains("MainWindow/geometry"))
    {
        restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    }
    if (settings.contains("MainWindow/windowState"))
    {
        restoreState(settings.value("MainWindow/windowState").toByteArray());
    }
    if (settings.contains("Settings/alpha"))
    {
        syncAlphaState(settings.value("Settings/alpha").toBool());
    }
    if (settings.contains("Settings/dithering"))
    {
        syncDitherState(settings.value("Settings/dithering").toBool());
    }
}

void MainWindow::updateToolbarWidgetsHeight()
{
    int btn_h = 38;
    if (QWidget * ref_btn = ui->drawToolBar->widgetForAction(ui->actionPencil))
    {
        btn_h = qMax(36, ref_btn->sizeHint().height());
    }
    if (QWidget * fill_btn = ui->drawToolBar->widgetForAction(ui->actionFill))
    {
        fill_btn->setFixedHeight(btn_h);
    }
    if (toolbarPresetBox)
    {
        toolbarPresetBox->setFixedHeight(btn_h);
    }
    if (toolbarReimportBtn)
    {
        toolbarReimportBtn->setFixedHeight(btn_h);
    }
    ui->importWizardApply->setFixedHeight(btn_h);
}

void MainWindow::syncPresetsUI(const QString & selectedPreset)
{
    const QString target = !selectedPreset.isEmpty() ? selectedPreset : ui->presetBox->currentText();
    const QStringList presets = QDir(Zx4ImportPars::defaultPresetPath()).entryList(QDir::Files);

    QSignalBlocker b1(ui->presetBox);
    QSignalBlocker b2(toolbarPresetBox);

    ui->presetBox->clear();
    toolbarPresetBox->clear();
    ui->menuPreset->clear();

    for (const QString & preset_name : presets)
    {
        ui->presetBox->addItem(preset_name);
        toolbarPresetBox->addItem(preset_name);

        QAction * act = ui->menuPreset->addAction(preset_name);
        act->setCheckable(true);
        presetMenuGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, preset_name]()
                {
                    const int idx = ui->presetBox->findText(preset_name);
                    if (idx >= 0)
                    {
                        ui->presetBox->setCurrentIndex(idx);
                    }
                });
    }

    int idx = ui->presetBox->findText(target);
    if (idx < 0)
    {
        idx = ui->presetBox->findText("Default");
    }
    if (idx >= 0)
    {
        ui->presetBox->setCurrentIndex(idx);
        toolbarPresetBox->setCurrentIndex(idx);
        const QList<QAction *> acts = presetMenuGroup->actions();
        if (idx < acts.size())
        {
            acts[idx]->setChecked(true);
        }
    }
}

void MainWindow::syncAlphaState(bool checked)
{
    QSignalBlocker b(ui->alphaBox);
    ui->alphaBox->setChecked(checked);
}

void MainWindow::syncDitherState(bool checked)
{
    QSignalBlocker b1(ui->ditherBox);
    QSignalBlocker b2(toolbarDitherBox);
    QSignalBlocker b3(ui->actionDither);

    ui->ditherBox->setChecked(checked);
    if (toolbarDitherBox)
    {
        toolbarDitherBox->setChecked(checked);
    }
    ui->actionDither->setChecked(checked);
}

bool MainWindow::askResizeSize(const QString & title, QSize & ioSize)
{
    QDialog dlg(this, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg.setWindowTitle(title);

    QFormLayout * form = new QFormLayout(&dlg);

    QSpinBox * widthSpin = new QSpinBox(&dlg);
    widthSpin->setRange(1, 255);
    widthSpin->setValue(ioSize.width());

    QSpinBox * heightSpin = new QSpinBox(&dlg);
    heightSpin->setRange(1, 192);
    heightSpin->setValue(ioSize.height());

    form->addRow("Width:", widthSpin);
    form->addRow("Height:", heightSpin);

    QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted)
    {
        return false;
    }

    ioSize = QSize(widthSpin->value(), heightSpin->value());
    return true;
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
    if (current_sprite) current_sprite->setCurrentColorIndex(0);
    if (current_scene)  current_scene->setCurrentColorIndex(0);
}

void MainWindow::on_actionColor_01_toggled(bool)
{
    if (current_sprite) current_sprite->setCurrentColorIndex(1);
    if (current_scene)  current_scene->setCurrentColorIndex(1);
}

void MainWindow::on_actionColor_10_toggled(bool)
{
    if (current_sprite) current_sprite->setCurrentColorIndex(2);
    if (current_scene)  current_scene->setCurrentColorIndex(2);
}

void MainWindow::on_actionColor_11_toggled(bool)
{
    if (current_sprite) current_sprite->setCurrentColorIndex(3);
    if (current_scene)  current_scene->setCurrentColorIndex(3);
}

void MainWindow::on_actionResize_canvas_triggered()
{
    if (!current_sprite)
    {
        return;
    }
    QSize new_size = current_sprite->canvasSize();
    if (!askResizeSize("Resize canvas", new_size))
    {
        return;
    }
    current_sprite->setCanvasSize(new_size);
    current_sprite->storeToHistory("Resize canvas");
    updateUndoRedoMenu(current_sprite);
}

void MainWindow::on_actionResize_image_triggered()
{
    if (!current_sprite)
    {
        return;
    }
    QSize new_size = current_sprite->canvasSize();
    if (!askResizeSize("Resize image", new_size))
    {
        return;
    }
    current_sprite->resizeImage(new_size);
    current_sprite->storeToHistory("Resize image");
    updateUndoRedoMenu(current_sprite);
}

void MainWindow::on_palettes_itemClicked(QListWidgetItem *)
{
    on_palettes_itemSelectionChanged();
}

void MainWindow::on_actionNew_triggered()
{
    SpriteWindow * new_sprite = new SpriteWindow(palettes);
    Zx4ImportPars p;
    importParsFromWidget(p);
    new_sprite->setImportPars(p);
    new_sprite->setAction(SpriteWindow::ActionPencil, ui->actionPencil);
    new_sprite->setAction(SpriteWindow::ActionBrush, ui->actionBrush);
    new_sprite->setAction(SpriteWindow::ActionFill, ui->actionFill);
    new_sprite->setAction(SpriteWindow::ActionMove, ui->actionMove);
    new_sprite->setAction(SpriteWindow::ActionEraser, ui->actionEraser);
    new_sprite->setBrushSize(current_brush_size);
    new_sprite->setEraserSize(current_eraser_size);
    new_sprite->image(0); // init 1 frame
    new_sprite->clearCanvas(QSize(64, 64));
    connect(new_sprite, &SpriteWindow::previewUpdateIsNeeded, ui->preview, qOverload<>(&QWidget::update));
    connect(new_sprite, &SpriteWindow::undoRedoMenuUpdateIsNeeded, this, &MainWindow::undoRedoMenuUpdate);
    new_sprite->storeToHistory("New sprite");

    QMdiSubWindow * msw = ui->mdiArea->addSubWindow(new_sprite);
    msw->show();
    current_sprite = new_sprite;
    updateUndoRedoMenu(current_sprite);
}

void MainWindow::on_actionImportWizardApply_triggered()
{
    ui->paletteNumberBox->setToolTip(QString());
    if (current_sprite)
    {
        importParsFromWidgetToObject(*current_sprite);
        reimport(current_sprite);
        current_sprite->storeToHistory("Import sprite");
        updateUndoRedoMenu(current_sprite);
    }
    else if (current_scene)
    {
        importParsFromWidgetToObject(*current_scene);
        reimport(current_scene);
        current_scene->storeToHistory("Import scene");
        updateUndoRedoMenu(current_scene);
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
    QString path = (current_sprite && !current_sprite->fileName().isEmpty()) ? QFileInfo(current_sprite->fileName()).absolutePath() : QString();
    if (path.isEmpty())
    {
        path = QDir::homePath() + "/Pictures/";
    }
    QString filter;
    QStringList filters = { "Images (*.png *.apng *.webp *.jpg *.jpeg *.jfif *.gif *.bmp)", "ZX4 Pixelizer sprites (*.zx4)" };
    QString filename = QFileDialog::getOpenFileName(this, "Import file", path, filters.join(";;"), &filter);
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
    else if (filters.indexOf(filter) == 1)
    {
        importSpriteZx4Format(filename);
    }
}

void MainWindow::on_actionExport_triggered()
{
    if (!current_sprite || current_sprite->framesCount() <= 0)
    {
        return;
    }
    QString path = !current_sprite->fileName().isEmpty() ? QFileInfo(current_sprite->fileName()).absolutePath() : QString();
    if (!path.isEmpty())
    {
        path += "/" + QFileInfo(current_sprite->fileName()).baseName();
    }
    else
    {
        path = QDir::homePath() + "/Pictures/";
    }
    QString filter;
    QStringList filters = { "ZX4 Pixelizer sprites (*.zx4)", "ZX Spectrum tape (*.tap)" };
    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, filters.join(";;"), &filter);
    if (filename.isEmpty())
    {
        return;
    }
    if (filters.indexOf(filter) == 0)
    {
        exportSpriteZx4Format(filename);
    }
    else if (filters.indexOf(filter) == 1)
    {
        Zx4ImageWithAttributes scene(256, 192);
        for (int y = 0; y < scene.height(); y++)
        {
            for (int x = 0; x < scene.width(); x++)
            {
                scene.alpha_layer[x][y] = Zx4Image::BitOn;
                scene.color_layer0[x][y] = static_cast<Zx4Image::Bit>((y + x / 2) % 2);
                scene.color_layer1[x][y] = static_cast<Zx4Image::Bit>((y + x) % 2);
            }
        }
        int k = 0;
        for (int y = 0; y < scene.height(); y += current_sprite->canvasSize().height())
        {
            for (int x = 0; x < scene.width(); x += current_sprite->canvasSize().width())
            {
                Zx4ImageWithAttributes image(current_sprite->image(k % current_sprite->framesCount()), current_sprite->paletteIndex());
                scene.copy(image, QPoint(x, y));
                k++;
            }
        }
        exportTape(scene, filename);
    }
}

void MainWindow::on_actionOpen_triggered()
{
    QString path;
    SpriteWindow * _current_sprite = current_sprite;
    if (_current_sprite && !_current_sprite->fileName().isEmpty())
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
    if (list.count() == 1 && list[0].text("ZX4_Frames").toInt() > 1)
    {
        QImage sheet = list[0];
        int frames = sheet.text("ZX4_Frames").toInt();
        int frame_h = sheet.height() / frames;
        list.clear();
        for (int k = 0; k < frames; k++)
        {
            QImage frame = sheet.copy(0, k * frame_h, sheet.width(), frame_h);
            frame.setColorTable(sheet.colorTable());
            list << frame;
        }
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
    QString path = !_current_sprite->fileName().isEmpty() ? QFileInfo(_current_sprite->fileName()).absolutePath() : QString();
    if (!path.isEmpty())
    {
        path += "/" + QFileInfo(_current_sprite->fileName()).baseName();
    }
    else
    {
        path = QDir::homePath() + "/Pictures/";
    }
    const bool has_apng = QImageWriter::supportedImageFormats().contains("apng");
    QString ext = (_current_sprite->framesCount() <= 1 || !has_apng) ? "zx4.png" : "zx4.apng";
    QString filter;
    QStringList filters = { "ZX4 Pixelizer images (*." + ext + ")" };
    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, filters.join(";;"), &filter);
    if (filename.isEmpty())
    {
        return;
    }
    while (filename.endsWith("." + ext, Qt::CaseInsensitive))
    {
        filename.chop(ext.length() + 1);
    }
    filename += "." + ext;
    _current_sprite->setFileName(filename);
    ui->actionSave->trigger();
}

void MainWindow::on_actionSave_triggered()
{
    SpriteWindow * _current_sprite = current_sprite;
    if (!_current_sprite || _current_sprite->framesCount() <= 0)
    {
        return;
    }
    if (_current_sprite->fileName().isEmpty())
    {
        ui->actionSave_as->trigger();
        return;
    }
    int p_idx = _current_sprite->paletteIndex();
    if (p_idx < 0 || p_idx >= palettes.palettes.count())
    {
        p_idx = std::clamp(ui->palettes->currentRow(), 0, int(palettes.palettes.count() - 1));
    }
    QVector<QRgb> color_table;
    for (const QColor & color : palettes.palettes[p_idx])
    {
        color_table << color.rgb();
    }
    color_table << 0; // alpha

    const bool use_apng = _current_sprite->framesCount() > 1 &&
                          _current_sprite->fileName().endsWith(".apng", Qt::CaseInsensitive) &&
                          QImageWriter::supportedImageFormats().contains("apng");

    if (use_apng || _current_sprite->framesCount() == 1)
    {
        QImageWriter iw(_current_sprite->fileName(), use_apng ? "apng" : "png");
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
    else
    {
        const int w = _current_sprite->canvasSize().width();
        const int h = _current_sprite->canvasSize().height();
        const int frames = _current_sprite->framesCount();
        QImage sheet(w, h * frames, QImage::Format_Indexed8);
        sheet.setColorTable(color_table);
        sheet.setText("ZX4_Frames", QString::number(frames));
        for (int k = 0; k < frames; k++)
        {
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    if (!_current_sprite->image(k).alpha_layer[x][y])
                    {
                        sheet.setPixel(x, k * h + y, 4);
                    }
                    else
                    {
                        sheet.setPixel(x, k * h + y, _current_sprite->colorIndex(_current_sprite->image(k), x, y));
                    }
                }
            }
        }
        QImageWriter iw(_current_sprite->fileName(), "png");
        if (!iw.write(sheet))
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
    QString path = (current_scene && !current_scene->fileName().isEmpty()) ? QFileInfo(current_scene->fileName()).absolutePath() : QString();
    if (path.isEmpty())
    {
        path = QDir::homePath() + "/Pictures/";
    }
    QString filename = QFileDialog::getOpenFileName(this, "Import file", path, "Images (*.png *.webp *.jpg *.jpeg *.jfif *.gif *.bmp)");
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
    const Zx4Image & scene = current_scene->image();
    auto scene_attr = current_scene->attr();
    if (scene.isNull() || scene_attr.isEmpty())
    {
        return;
    }
    QString path = !current_scene->fileName().isEmpty() ? QFileInfo(current_scene->fileName()).absolutePath() : QString();
    if (!path.isEmpty())
    {
        path += "/" + QFileInfo(current_scene->fileName()).baseName();
    }
    else
    {
        path = QDir::homePath() + "/Pictures/";
    }
    QString filter;
    QStringList filters = { "ZX Spectrum screen (*.scr0)", "ZX Spectrum tape (*.tap)", "PNG image (*.png)" };
    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, filters.join(";;"), &filter);
    if (filename.isEmpty())
    {
        return;
    }
    const Zx4ImageWithAttributes * scene_with_attr = dynamic_cast<const Zx4ImageWithAttributes *>(&current_scene->image());
    if (!scene_with_attr)
    {
        QMessageBox::critical(this, "Error", "Error occured while exporting scene \"" + QFileInfo(filename).fileName() + "\"");
        return;
    }
    if (filters.indexOf(filter) == 2)
    {
        if (!filename.endsWith(".png", Qt::CaseInsensitive))
        {
            filename += ".png";
        }
        current_scene->renderedImage().convertToFormat(QImage::Format_RGB32).save(filename, "PNG");
    }
    else if (filters.indexOf(filter) == 1)
    {
        exportTape(*scene_with_attr, filename);
    }
    else if (filters.indexOf(filter) == 0)
    {
        exportScreens(*scene_with_attr, filename);
    }
}

void MainWindow::on_actionAbout_ZX4_Pixelizer_triggered()
{
    QMessageBox::about(this, "ZX4 Pixelizer", QString("ZX4 Pixelizer\nImages & sprites manipulation tool\nfor ZX Spectrum 128K\n\nVersion %1\n\n© 2022-2026\nZXLDR - ZX Lab of Dreams Reinvention").arg(APP_VERSION));
}

void MainWindow::on_mdiArea_subWindowActivated(QMdiSubWindow *msw)
{
    if (current_sprite)
    {
        importParsFromWidgetToObject(*current_sprite);
    }
    if (current_scene)
    {
        importParsFromWidgetToObject(*current_scene);
    }
    current_sprite = nullptr;
    current_scene = nullptr;
    if (!msw)
    {
        updateUndoRedoMenu(nullptr);
        ui->preview->update();
        return;
    }
    if (SpriteWindow * sw = dynamic_cast<SpriteWindow *>(msw->widget()))
    {
        current_sprite = sw;
        current_sprite->setBrushSize(current_brush_size);
        current_sprite->setEraserSize(current_eraser_size);
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
        importParsFromObjectToWidget(*current_sprite);
    }
    else if (SceneWindow * sw = dynamic_cast<SceneWindow *>(msw->widget()))
    {
        current_scene = sw;
        importParsFromObjectToWidget(*current_scene);
    }
    updateUndoRedoMenu(dynamic_cast<Zx4BaseObject *>(msw->widget()));
    ui->preview->update();
}

void MainWindow::updateUndoRedoMenu(Zx4BaseObject * zbo)
{
    ui->actionUndo->setEnabled(false);
    ui->actionRedo->setEnabled(false);
    ui->actionUndo->setText(QString("Undo"));
    ui->actionRedo->setText(QString("Redo"));

    const bool has_sprite = (current_sprite != nullptr && current_sprite->framesCount() > 0);
    ui->actionCut->setEnabled(has_sprite);
    ui->actionCopy->setEnabled(has_sprite);
    ui->actionPaste->setEnabled(current_sprite != nullptr && !clipboard_frame.isNull());
    ui->actionClear->setEnabled(has_sprite);

    if (!zbo)
    {
        return;
    }
    QString desc;
    if (zbo->canUndo(&desc))
    {
        ui->actionUndo->setEnabled(true);
        ui->actionUndo->setText(QString("Undo \"%1\"").arg(desc));
    }
    if (zbo->canRedo(&desc))
    {
        ui->actionRedo->setEnabled(true);
        ui->actionRedo->setText(QString("Redo \"%1\"").arg(desc));
    }
}

void MainWindow::selectPalette(int p_idx)
{
    if (p_idx < 0 || p_idx >= palettes.palettes.count())
    {
        return;
    }
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

void MainWindow::on_presetBox_currentIndexChanged(int index)
{
    if (toolbarPresetBox && index >= 0 && toolbarPresetBox->currentIndex() != index)
    {
        QSignalBlocker b(toolbarPresetBox);
        toolbarPresetBox->setCurrentIndex(index);
    }
    if (presetMenuGroup && index >= 0)
    {
        const QList<QAction *> acts = presetMenuGroup->actions();
        if (index < acts.size())
        {
            acts[index]->setChecked(true);
        }
    }
    if (ui->presetBox->currentText().isEmpty())
    {
        return;
    }
    Zx4ImportPars p;
    QFile file(Zx4ImportPars::defaultPresetPath() + "/" + ui->presetBox->currentText());
    if (!file.open(QFile::ReadOnly))
    {
        return;
    }
    if (!p.deserialize(file.readAll()))
    {
        file.close();
        return;
    }
    file.close();
    importParsToWidget(p);
    if (current_sprite)
    {
        current_sprite->setImportPars(p);
    }
    if (current_scene)
    {
        current_scene->setImportPars(p);
    }
    ui->preview->update();
}

void MainWindow::on_addPreset_clicked()
{
    const QString name = ui->presetBox->currentText().trimmed();
    if (name.isEmpty())
    {
        return;
    }
    Zx4ImportPars p;
    importParsFromWidget(p);
    QDir().mkpath(Zx4ImportPars::defaultPresetPath());
    QFile file(Zx4ImportPars::defaultPresetPath() + "/" + name);
    if (file.open(QFile::WriteOnly))
    {
        file.write(p.serialize());
        file.close();
        syncPresetsUI(name);
    }
}

void MainWindow::on_removePreset_clicked()
{
    const QString name = ui->presetBox->currentText();
    if (name.isEmpty())
    {
        return;
    }
    QFile::remove(Zx4ImportPars::defaultPresetPath() + "/" + name);
    syncPresetsUI("Default");
    on_presetBox_currentIndexChanged(ui->presetBox->currentIndex());
}

void MainWindow::on_actionUndo_triggered()
{
    if (ui->mdiArea->activeSubWindow())
    {
        if (auto zbo = dynamic_cast<Zx4BaseObject *>(ui->mdiArea->activeSubWindow()->widget()))
        {
            zbo->undo();
            undoRedoMenuUpdate();
        }
    }
}

void MainWindow::on_actionRedo_triggered()
{
    if (ui->mdiArea->activeSubWindow())
    {
        if (auto zbo = dynamic_cast<Zx4BaseObject *>(ui->mdiArea->activeSubWindow()->widget()))
        {
            zbo->redo();
            undoRedoMenuUpdate();
        }
    }
}

void MainWindow::undoRedoMenuUpdate()
{
    if (!ui->mdiArea->activeSubWindow())
    {
        return;
    }
    if (auto zbo = dynamic_cast<Zx4BaseObject *>(ui->mdiArea->activeSubWindow()->widget()))
    {
        updateUndoRedoMenu(zbo);
        zbo->canvas()->update();
        ui->preview->update();
    }
}

void MainWindow::on_actionCut_triggered()
{
    if (!current_sprite || current_sprite->framesCount() <= 0)
    {
        return;
    }
    clipboard_frame.reset(current_sprite->image(current_sprite->currentFrame()).clone());
    current_sprite->cutCurrentFrame();
    updateUndoRedoMenu(current_sprite);
}

void MainWindow::on_actionCopy_triggered()
{
    if (!current_sprite || current_sprite->framesCount() <= 0)
    {
        return;
    }
    clipboard_frame.reset(current_sprite->image(current_sprite->currentFrame()).clone());
    updateUndoRedoMenu(current_sprite);
}

void MainWindow::on_actionPaste_triggered()
{
    if (!current_sprite || clipboard_frame.isNull())
    {
        return;
    }
    const int insert_idx = current_sprite->currentFrame() + 1;
    current_sprite->insertFrameAt(insert_idx, *clipboard_frame, "Paste frame");
    updateUndoRedoMenu(current_sprite);
}

void MainWindow::on_actionClear_triggered()
{
    if (!current_sprite || current_sprite->framesCount() <= 0)
    {
        return;
    }
    current_sprite->clearCurrentFrame("Clear frame");
    updateUndoRedoMenu(current_sprite);
}
