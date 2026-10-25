#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lz4.h"
#include "lz4hc.h"

#include <QPainter>
#include <QResizeEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QIntValidator>
#include <QDebug>
#include <QMessageBox>
#include <QProgressDialog>
#include <QMimeData>
#include <algorithm>
#ifdef _OPENMP
#include <omp.h>
#endif

void MainWindow::createPalettes()
{
    palettes.init();
    for (int p_idx = 0; p_idx < palettes.palettes.count(); p_idx++)
    {
        if (p_idx % 50 == 0)
        {
            qApp->processEvents();
        }
        QList<QColor> m = palettes.palettes[p_idx];
        QPixmap px(64, 64);
        QPainter p(&px);
        p.fillRect(0, 0, px.width() / 2, px.height() / 2,                            m[0]);
        p.fillRect(px.width() / 2, 0, px.width() / 2, px.height() / 2,               m[1]);
        p.fillRect(0, px.height() / 2, px.width() / 2, px.height() / 2,              m[2]);
        p.fillRect(px.width() / 2, px.height() / 2, px.width() / 2, px.height() / 2, m[3]);

        QList<ZxColorEnum> z = palettes.color_palette_table[p_idx];
        QListWidgetItem * item = new QListWidgetItem();
        item->setData(Qt::UserRole, p_idx);
        item->setToolTip((QStringList() << QString("#%1").arg(p_idx + 1)
                                        << palettes.color_name[z[0]]
                                        << palettes.color_name[z[1]]
                                        << palettes.color_name[z[2]]
                                        << palettes.color_name[z[3]]).join("\n"));
        item->setIcon(px);
        ui->palettes->addItem(item);
        ui->paletteIndex->addItem(px, QString::number(p_idx + 1));
    }
    ui->paletteIndex->lineEdit()->setValidator(new QIntValidator(1, palettes.palettes.count(), this));
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
    for (const QColor & color : palettes.palettes[palette_number])
    {
        color_table << color.rgb();
    }
    return Zx4Processing::importImage(img, color_table, use_alpha);
}

Zx4Palette::SelectPaletteMethod MainWindow::selectPaletteMethod() const
{
    return static_cast<Zx4Palette::SelectPaletteMethod>(ui->colorPickBox->currentIndex());
}

QImage MainWindow::applyTransforms(const Zx4ImportPars & p, const QImage & src, QColor bg_color) const
{
    int mirror = p.horizontal_mirror ? -1 : 1;
    QImage img(src.size(), src.format());
    img.fill(bg_color);
    QPainter pmx(&img);
    QTransform mx;
    mx.translate(p.x_shift, p.y_shift);
    mx.translate(img.width() / 2.0, img.height() / 2.0);
    mx.rotate(p.rotate);
    mx.translate(-img.width() / 2.0, -img.height() / 2.0);
    mx.scale(p.x_scale * mirror, p.y_scale);
    pmx.setTransform(mx);

    QImage color_shifted = src;
    if (p.hue_shift != 0 || p.saturation_shift != 0 || p.lightness_shift != 0)
    {
        const float ls = static_cast<float>(p.lightness_shift);
        const float ss = static_cast<float>(p.saturation_shift);
        const float hs = static_cast<float>(p.hue_shift);

        for (int y = 0; y < color_shifted.height(); ++y)
        {
            for (int x = 0; x < color_shifted.width(); ++x)
            {
                QColor c = color_shifted.pixelColor(x, y).toHsl();
                float h = 0.0f, s = 0.0f, l = 0.0f;
                c.getHslF(&h, &s, &l);
                if (h >= 0.0f)
                {
                    h += hs;
                    while (h < 0.0f) h += 1.0f;
                    while (h > 1.0f) h -= 1.0f;
                }

                // Сначала корректируем яркость (при отрицательном ls уводим темные шумы в чистый 0)
                if (ls < 0.0f)
                {
                    float black_cut = -ls * 0.55f;
                    if (l <= black_cut)
                    {
                        l = 0.0f;
                        s = 0.0f;
                    }
                    else
                    {
                        l = std::pow((l - black_cut) / (1.0f - black_cut), 1.0f - ls * 0.75f);
                    }
                }
                else if (ls > 0.0f)
                {
                    l = std::clamp(l + ls, 0.0f, 1.0f);
                }

                // Усиливаем насыщенность пропорционально текущей яркости и цветности,
                // чтобы почти чёрные и нейтрально-серые тени не взрывались лиловым шумом
                if (ss > 0.0f)
                {
                    float shadow_gate = std::clamp(l / 0.22f, 0.0f, 1.0f);
                    float chroma_gate = std::clamp(s / 0.12f, 0.0f, 1.0f);
                    s = std::clamp(s + ss * shadow_gate * chroma_gate, 0.0f, 1.0f);
                }
                else if (ss < 0.0f)
                {
                    s = std::clamp(s + ss, 0.0f, 1.0f);
                }

                c.setHslF(h, s, std::clamp(l, 0.0f, 1.0f));
                color_shifted.setPixelColor(x, y, c);
            }
        }
    }
    pmx.drawImage(QPoint(mirror > 0 ? 0 : -img.width(), 0), color_shifted);
    return img;
}

void MainWindow::importScene(QImage img, QString filename)
{
    img = img.convertToFormat(QImage::Format_RGB32);
    if (img.isNull())
    {
        QMessageBox::critical(this, "Error", QString("Error occured while opening \"%1\"").arg(filename));
        return;
    }

    current_scene = new SceneWindow(palettes);
    importParsFromObjectToWidget(*current_scene);
    ui->mdiArea->addSubWindow(current_scene);
    current_scene->show();
    connect(current_scene, &SceneWindow::charSelected, this, &MainWindow::selectPalette, Qt::QueuedConnection);
    connect(current_scene, &SceneWindow::previewUpdateIsNeeded, ui->preview, qOverload<>(&QWidget::update));
    connect(current_scene, &SceneWindow::previewUpdateIsNeeded, ui->sourceImage, qOverload<>(&QWidget::update));
    connect(current_scene, &SceneWindow::undoRedoMenuUpdateIsNeeded, this, &MainWindow::undoRedoMenuUpdate);

    current_scene->setFileName(filename);
    img = img.scaled(qMin(((img.width() + 7) / 8) * 8, 256), qMin(((img.height() + 7) / 8) * 8, 192), Qt::KeepAspectRatio);
    QImage dst_img(256, 192, img.format());
    dst_img.fill(0);
    QPainter p(&dst_img);
    p.drawImage((dst_img.width() - img.width()) / 2, (dst_img.height() - img.height()) / 2, img);

    current_scene->Zx4BaseObject::setSourceImage(dst_img);
    reimport(current_scene);
    current_scene->clearHistory();
    current_scene->storeToHistory("Import scene");
    updateUndoRedoMenu(current_scene);
}

void MainWindow::openSprite(QList<QImage> src_imp_image, QString filename)
{
    if (src_imp_image.isEmpty() || src_imp_image[0].isNull())
    {
        QMessageBox::critical(this, "Error", QString("Error occured while opening \"%1\"").arg(filename));
        return;
    }
    if (src_imp_image[0].width() > 255 || src_imp_image[0].height() > 192)
    {
        QMessageBox::critical(this, "Error", "Wrong image size");
        return;
    }
    if (src_imp_image[0].colorTable().count() != 5)
    {
        QMessageBox::critical(this, "Error", "Wrong image color table");
        return;
    }
    int p_idx = palettes.exactPalette(src_imp_image[0]);
    if (p_idx < 0)
    {
        p_idx = ui->palettes->currentRow();
    }
    if (!current_sprite)
    {
        ui->actionNew->trigger();
    }
    SpriteWindow * _current_sprite = current_sprite;
    selectPalette(p_idx);
    QProgressDialog * pd = createProgress(this, "Opening sprite");
    pd->setRange(0, src_imp_image.count() - 1);
    _current_sprite->setPaletteIndex(p_idx);
    _current_sprite->clearCanvas(QSize(qMin(src_imp_image[0].width(), 255), qMin(src_imp_image[0].height(), 192)));
    _current_sprite->setFileName(filename);
    _current_sprite->setSourceImage(src_imp_image);
    for (int k = 0; k < _current_sprite->framesCount(); k++)
    {
        _current_sprite->setCurrentFrame(k);
        _current_sprite->drawImage(_current_sprite->sourceImage(k));
        pd->setValue(k);
        qApp->processEvents();
    }
    _current_sprite->setCurrentFrame(0);
    _current_sprite->drawImage(_current_sprite->sourceImage(0));
    pd->hide();
    pd->deleteLater();
    _current_sprite->clearHistory();
    _current_sprite->storeToHistory("Open sprite");
    updateUndoRedoMenu(_current_sprite);
}

void MainWindow::importSprite(QList<QImage> src_imp_image, QString filename, QSize sz, int p_idx)
{
    if (src_imp_image.isEmpty() || src_imp_image[0].isNull())
    {
        QMessageBox::critical(this, "Error", QString("Error occured while opening \"%1\"").arg(filename));
        return;
    }
    ui->actionNew->trigger();
    if (!sz.isValid())
    {
        ui->actionResize_canvas->trigger();
    }
    else
    {
        current_sprite->setCanvasSize(sz);
    }
    current_sprite->setFileName(filename);
    current_sprite->setSourceImage(src_imp_image);
    reimport(current_sprite, p_idx);
    current_sprite->clearHistory();
    current_sprite->storeToHistory("Import sprite");
    updateUndoRedoMenu(current_sprite);
}

void MainWindow::importSpriteZx4Format(QString filename)
{
    QByteArray zx4;
    QFile f(filename);
    if (f.open(QFile::ReadOnly))
    {
        zx4 = f.readAll();
        f.close();
    }
    if (zx4.size() <= 5)
    {
        QMessageBox::critical(this, "Error", QString("Error occured while opening \"%1\"").arg(filename));
        return;
    }
    const quint8 b0 = static_cast<quint8>(zx4[0]);
    const quint8 b1 = static_cast<quint8>(zx4[1]);
    QList<ZxColorEnum> z;
    z << static_cast<ZxColorEnum>((b0 & 0x78) >> 3);
    z << static_cast<ZxColorEnum>((b0 & 0x07) | ((b0 & 0x40) >> 3));
    z << static_cast<ZxColorEnum>((b1 & 0x78) >> 3);
    z << static_cast<ZxColorEnum>((b1 & 0x07) | ((b1 & 0x40) >> 3));
    int p_idx = palettes.color_palette_table.key(z, -1);
    if (p_idx == -1)
    {
        QMessageBox::critical(this, "Error", QString("Error occured while applying palette for \"%1\"").arg(filename));
        return;
    }
    ui->actionNew->trigger();
    selectPalette(p_idx);
    current_sprite->setPaletteIndex(p_idx);

    const int width = static_cast<quint8>(zx4[2]);
    const int height = static_cast<quint8>(zx4[3]);
    const int frames_count = static_cast<quint8>(zx4[4]);
    if (width <= 0 || height <= 0 || frames_count <= 0)
    {
        QMessageBox::critical(this, "Error", QString("Corrupted header in \"%1\"").arg(filename));
        return;
    }
    current_sprite->setCanvasSize(QSize(width, height));

    QByteArray zx4_data = zx4.mid(5);
#ifndef USE_RLE8
    QByteArray zx4_lz4 = zx4_data;
    const int max_layer_bytes = ((width * height + 7) / 8) * frames_count;
    zx4.resize(3 * max_layer_bytes);
    int isize = zx4_lz4.count();
    int osize = ::LZ4_uncompress_unknownOutputSize(zx4_lz4.constData(), zx4.data(), &isize, zx4.count());
    if (osize <= 0)
    {
        QMessageBox::critical(this, "Error", QString("Error occured while decompressing \"%1\"").arg(filename));
        return;
    }
    zx4.resize(osize);
#else
    zx4 = Zx4Processing::inflateRle8(zx4_data);
#endif
    QList<const Zx4Image *> list;
    for (int k = 0; k < frames_count; k++)
    {
        auto zx4_image = new Zx4Image();
        zx4_image->clear(width, height);
        list << zx4_image;
    }
    current_sprite->setImage(list);
    qDeleteAll(list);
    list.clear();

    int alpha_bit = 0;
    for (int k = 0; k < frames_count; k++)
    {
        Zx4Image & zx4_image = const_cast<Zx4Image &>(current_sprite->image(k));
        for (int x = 0; x < (width + 7) / 8; x++)
        {
            for (int y = 0; y < height; y++)
            {
                auto & alpha = zx4_image.alpha_layer;
                for (int bit = 0; bit < 8; bit++)
                {
                    const int px = (8 * x) + (7 - bit);
                    if (px < width)
                    {
                        alpha[px][y] = static_cast<Zx4Image::Bit>((static_cast<quint8>(zx4[alpha_bit / 8]) >> (alpha_bit % 8)) & 0x01);
                        ++alpha_bit;
                    }
                }
            }
        }
    }
    zx4 = zx4.mid((alpha_bit + 7) / 8);
    int color_bit = 0;
    QByteArray zx4_color0 = zx4.left(zx4.count() / 2), zx4_color1 = zx4.mid(zx4.count() / 2);
    for (int k = 0; k < frames_count; k++)
    {
        Zx4Image & zx4_image = const_cast<Zx4Image &>(current_sprite->image(k));
        for (int x = 0; x < (width + 7) / 8; x++)
        {
            for (int y = 0; y < height; y++)
            {
                auto & alpha = zx4_image.alpha_layer;
                auto & layer0 = zx4_image.color_layer0;
                auto & layer1 = zx4_image.color_layer1;
                for (int bit = 0; bit < 8; bit++)
                {
                    const int px = (8 * x) + (7 - bit);
                    if (px < width && alpha[px][y])
                    {
                        layer0[px][y] = static_cast<Zx4Image::Bit>((static_cast<quint8>(zx4_color0[color_bit / 8]) >> (color_bit % 8)) & 0x01);
                        layer1[px][y] = static_cast<Zx4Image::Bit>((static_cast<quint8>(zx4_color1[color_bit / 8]) >> (color_bit % 8)) & 0x01);
                        ++color_bit;
                    }
                }
            }
        }
    }
    current_sprite->update();
    current_sprite->clearHistory();
    current_sprite->storeToHistory("Import sprite");
    updateUndoRedoMenu(current_sprite);
}

void MainWindow::exportSpriteZx4Format(QString filename)
{
    if (filename.isEmpty() || !current_sprite)
    {
        return;
    }
    if (QFileInfo(filename).suffix().isEmpty())
    {
        filename += ".zx4";
    }
    int p_idx = ui->palettes->currentRow();
    QList<ZxColorEnum> z = palettes.color_palette_table[p_idx];
    const int width = current_sprite->canvasSize().width();
    const int height = current_sprite->canvasSize().height();
    const int layer_sz = ((width * height + 7) / 8) * current_sprite->framesCount();

    QByteArray zx4;
    // 4-color palette
    zx4 += static_cast<char>((z[1] & 0x07) | ((z[0] & 0x0F) << 3));
    zx4 += static_cast<char>((z[3] & 0x07) | ((z[2] & 0x0F) << 3));
    // width & height
    zx4 += static_cast<char>(width);
    zx4 += static_cast<char>(height);
    // frames count
    zx4 += static_cast<char>(current_sprite->framesCount());

    int alpha_bit = 0, color_bit = 0;
    QByteArray zx4_alpha(layer_sz, 0), zx4_color0(layer_sz, 0), zx4_color1(layer_sz, 0);
    for (int k = 0; k < current_sprite->framesCount(); k++)
    {
        const auto & zx4_image = current_sprite->image(k);
        for (int x = 0; x < (width + 7) / 8; x++)
        {
            for (int y = 0; y < height; y++)
            {
                const auto & alpha = zx4_image.alpha_layer;
                const auto & layer0 = zx4_image.color_layer0;
                const auto & layer1 = zx4_image.color_layer1;
                for (int bit = 0; bit < 8; bit++)
                {
                    const int px = (8 * x) + (7 - bit);
                    if (px < width)
                    {
                        auto alpha_value = alpha[px][y];
                        zx4_alpha[alpha_bit / 8] = static_cast<char>(zx4_alpha[alpha_bit / 8] | (static_cast<int>(alpha_value) << (alpha_bit % 8)));
                        if (alpha_value)
                        {
                            zx4_color0[color_bit / 8] = static_cast<char>(zx4_color0[color_bit / 8] | (static_cast<int>(layer0[px][y]) << (color_bit % 8)));
                            zx4_color1[color_bit / 8] = static_cast<char>(zx4_color1[color_bit / 8] | (static_cast<int>(layer1[px][y]) << (color_bit % 8)));
                            ++color_bit;
                        }
                        ++alpha_bit;
                    }
                }
            }
        }
    }
    zx4_alpha.resize((alpha_bit + 7) / 8);
    zx4_color0.resize((color_bit + 7) / 8);
    zx4_color1.resize((color_bit + 7) / 8);

    QByteArray zx4_data = zx4_alpha + zx4_color0 + zx4_color1;
#ifdef USE_RLE8
    zx4_data = Zx4Processing::deflateRle8(zx4_data);
#else
    const int zx4_lz4max_count = ::LZ4_compressBound(zx4_data.count());
    QByteArray zx4_lz4(zx4_lz4max_count, 0);
    const int zx4_count = ::LZ4_compressHC(zx4_data.constData(), zx4_lz4.data(), zx4_data.count());
    zx4_lz4.resize(zx4_count);
    zx4_data = zx4_lz4;
#endif
    zx4 += zx4_data;
    const int src_size = 5 + 3 * layer_sz;
    qDebug() << "sprite deflated from" << src_size << "to" << zx4.count() << ", total ratio" << 100.0 * zx4.count() / src_size << "%";
    QFile f(filename);
    if (f.open(QFile::WriteOnly))
    {
        f.write(zx4);
        f.close();
    }
}

void MainWindow::exportScreens(const Zx4ImageWithAttributes & image, QString filename_scr0, QString filename_scr1)
{
    QFileInfo fi0(filename_scr0);
    if (fi0.suffix().compare("scr0", Qt::CaseInsensitive) != 0)
    {
        filename_scr0 = fi0.path() + "/" + fi0.completeBaseName() + ".scr0";
    }
    if (filename_scr1.isEmpty())
    {
        filename_scr1 = filename_scr0;
    }
    QFileInfo fi1(filename_scr1);
    if (fi1.suffix().compare("scr1", Qt::CaseInsensitive) != 0)
    {
        filename_scr1 = fi1.path() + "/" + fi1.completeBaseName() + ".scr1";
    }
    auto screens = image.makeScreens(palettes);
    QFile f0(filename_scr0), f1(filename_scr1);
    if (f0.open(QFile::WriteOnly) && f1.open(QFile::WriteOnly))
    {
        f0.write(screens.first);
        f1.write(screens.second);
    }
    f0.close();
    f1.close();
}

void MainWindow::exportTape(const Zx4ImageWithAttributes & image, QString filename)
{
    if (!filename.endsWith(".tap", Qt::CaseInsensitive))
    {
        QFileInfo fi(filename);
        filename = fi.suffix().isEmpty() ? (filename + ".tap") : (fi.path() + "/" + fi.completeBaseName() + ".tap");
    }
    QFile f(filename);
    if (!f.open(QFile::WriteOnly))
    {
        return;
    }
    QFile ggf(":/GIGAVIEW.TAP");
    if (ggf.open(QFile::ReadOnly))
    {
        f.write(ggf.readAll());
        ggf.close();
    }
    auto screens = image.makeScreens(palettes);
    int offset = 0xC0;
    for (const QByteArray & scr : { screens.first, screens.second })
    {
        quint16 sz = static_cast<quint16>(scr.count());
        QByteArray h;
        h += static_cast<char>(0x13);
        h += static_cast<char>(0x00);
        h += static_cast<char>(0x00);
        h += static_cast<char>(0x03);
        h += "SCREEN    ";
        h += static_cast<char>(sz & 0xFF);
        h += static_cast<char>((sz >> 8) & 0xFF);
        h += static_cast<char>(0x00);
        h += static_cast<char>(offset);
        h += static_cast<char>(0x00);
        h += static_cast<char>(0x00);
        char checksum = h[2];
        for (int i = 3; i < h.count(); i++)
        {
            checksum ^= h[i];
        }
        h += checksum;
        offset += 0x1B;
        f.write(h);

        sz += 2;
        QByteArray d;
        d += static_cast<char>(sz & 0xFF);
        d += static_cast<char>((sz >> 8) & 0xFF);
        d += static_cast<char>(0xFF);
        d += scr;
        checksum = d[2];
        for (int i = 3; i < d.count(); i++)
        {
            checksum ^= d[i];
        }
        d += checksum;
        f.write(d);
    }
    f.close();
}

void MainWindow::importParsFromWidget(Zx4ImportPars & p)
{
    p.alpha = ui->alphaBox->isChecked();
    p.dithering = ui->ditherBox->isChecked();
    p.autopalette = ui->wizardPaletteBox->currentIndex() == 0;
    p.color_pick = static_cast<Zx4Palette::SelectPaletteMethod>(ui->colorPickBox->currentIndex());
    p.horizontal_mirror = ui->mirrored->isChecked();
    p.multipass = static_cast<Zx4ImportPars::PassMethod>(ui->multipass->currentIndex());
    p.palette_index_max = ui->paletteNumberBox->maximum();
    p.palette_index = ui->paletteNumberBox->value();
    p.rotate = ui->rotate->value();
    p.x_scale = ui->xScale->value();
    p.x_shift = ui->xShift->value();
    p.y_scale = ui->yScale->value();
    p.y_shift = ui->yShift->value();
    p.hue_shift = ui->hueShift->value();
    p.saturation_shift = ui->saturationShift->value();
    p.lightness_shift = ui->lightnessShift->value();
}

void MainWindow::importParsToWidget(const Zx4ImportPars & p)
{
    ui->alphaBox->setChecked(p.alpha);
    ui->ditherBox->setChecked(p.dithering);
    ui->wizardPaletteBox->setCurrentIndex(p.autopalette ? 0 : 1);
    ui->colorPickBox->setCurrentIndex(p.color_pick);
    ui->mirrored->setChecked(p.horizontal_mirror);
    ui->multipass->setCurrentIndex(p.multipass);
    ui->paletteNumberBox->setMaximum(qMax(1, p.palette_index_max));
    ui->paletteNumberBox->setValue(qMax(1, p.palette_index));
    ui->rotate->setValue(p.rotate);
    ui->xScale->setValue(p.x_scale);
    ui->xShift->setValue(p.x_shift);
    ui->yScale->setValue(p.y_scale);
    ui->yShift->setValue(p.y_shift);
    ui->hueShift->setValue(p.hue_shift);
    ui->saturationShift->setValue(p.saturation_shift);
    ui->lightnessShift->setValue(p.lightness_shift);
}

void MainWindow::importParsFromWidgetToObject(Zx4BaseObject & b)
{
    Zx4ImportPars p = b.importPars();
    importParsFromWidget(p);
    b.setImportPars(p);
}

void MainWindow::importParsFromObjectToWidget(const Zx4BaseObject & b)
{
    Zx4ImportPars p = b.importPars();
    importParsToWidget(p);
}

void MainWindow::reimport(SpriteWindow * _current_sprite, int p_idx)
{
    if (!_current_sprite || _current_sprite->framesCount() <= 0)
    {
        return;
    }
    QProgressDialog * pd = createProgress(this, "Importing sprite\n" + ui->colorPickBox->currentText());
    pd->setRange(0, _current_sprite->framesCount() - 1);
    if (p_idx < 0)
    {
        auto sel_method = selectPaletteMethod();
        p_idx = ui->palettes->currentRow();
        if (ui->wizardPaletteBox->currentIndex() == 0)
        {
            p_idx = palettes.exactPalette(_current_sprite->sourceImage());
            if (p_idx < 0)
            {
                QMultiMap<double, int> dist = palettes.selectPalette(_current_sprite->sourceImage(), sel_method);
                if (!dist.isEmpty())
                {
                    ui->paletteNumberBox->setRange(1, dist.count());
                    ui->paletteNumberBox->setToolTip(QString("%1..%2").arg(1).arg(dist.count()));
                    auto it = dist.begin();
                    std::advance(it, ui->paletteNumberBox->value() - 1);
                    p_idx = *it;
                }
            }
        }
    }
    _current_sprite->setPaletteIndex(p_idx);
    int current_frame = _current_sprite->currentFrame();
    for (int k = 0; k < _current_sprite->framesCount(); k++)
    {
        _current_sprite->setCurrentFrame(k);
        if (!_current_sprite->sourceImage(k).isNull())
        {
            _current_sprite->drawImage(importImage(applyTransforms(_current_sprite->importPars(), _current_sprite->sourceImage(k).convertToFormat(QImage::Format_ARGB32)), p_idx, ui->alphaBox->isChecked()));
        }
        pd->setValue(k);
        qApp->processEvents();
    }
    _current_sprite->setCurrentFrame(current_frame);
    _current_sprite->canvas()->update();
    pd->hide();
    pd->deleteLater();
}

void MainWindow::reimport(SceneWindow * _current_scene)
{
    if (!_current_scene)
    {
        return;
    }
    const auto pars = _current_scene->importPars();
    QProgressDialog * pd = createProgress(this, "Importing scene\n" + ui->colorPickBox->currentText());
    QImage img = applyTransforms(pars, _current_scene->sourceImage())
        .convertToFormat(pars.dithering ? QImage::Format_Indexed8 : QImage::Format_RGB32);
    _current_scene->transformedImage() = img;
    pd->setRange(0, (img.width() / 8) * (img.height() / 8));

    QImage out(img.width(), img.height(), QImage::Format_RGB32);
    for (QMdiSubWindow * msw : ui->mdiArea->subWindowList())
    {
        if (msw->widget() == _current_scene)
        {
            QPoint pos = _current_scene->canvas()->mapTo(msw, QPoint(0, 0));
            msw->resize(out.width() * 4 + pos.x(), out.height() * 4 + pos.y());
            break;
        }
    }

    int progressCounter = 0;
    const auto sel_method = selectPaletteMethod();
    int multipass_offset = 0;
    const Zx4ImportPars::PassMethod multipass = static_cast<Zx4ImportPars::PassMethod>(ui->multipass->currentIndex());
    switch (multipass)
    {
    case Zx4ImportPars::Multipass3x3x1:
        multipass_offset = 1;
        break;
    case Zx4ImportPars::Multipass5x5x1:
    case Zx4ImportPars::Multipass5x5x2:
        multipass_offset = 2;
        break;
    case Zx4ImportPars::Multipass7x7x1:
    case Zx4ImportPars::Multipass7x7x3:
        multipass_offset = 3;
        break;
    default:
        multipass_offset = 0;
        break;
    }

    const int width = img.width();
    const int height = img.height();
    const int color_variations = 1;
    const int current_palette = ui->palettes->currentRow();

    for (int y = 0; y < height; y += 8)
    {
#pragma omp parallel for schedule(dynamic)
        for (int x = 0; x < width; x += 8)
        {
#pragma omp atomic
            progressCounter++;

            int p_idx = -1;
            QImage small_quad = img.copy(QRect(x, y, 8, 8));
            if (_current_scene->importPars().autopalette)
            {
                if (multipass == Zx4ImportPars::SinglePass)
                {
                    p_idx = _current_scene->selectPalette(small_quad, sel_method, color_variations);
                }
                else
                {
                    // Передаем блок 8x8 вместе с расширенным контекстом соседних клеток,
                    // где центральные 8x8 пикселей имеют приоритетный вес (см. selectPalette)
                    int rx = qMax(0, x - multipass_offset);
                    int ry = qMax(0, y - multipass_offset);
                    int rw = qMin(width - rx, 8 + 2 * multipass_offset);
                    int rh = qMin(height - ry, 8 + 2 * multipass_offset);
                    QImage context_quad(rw, rh, QImage::Format_RGB32);
                    // Первыми (0..7, 0..7) кладем пиксели самого знакоместа, далее — пиксели окружения
                    QPainter cp(&context_quad);
                    cp.drawImage(0, 0, img, rx, ry, rw, rh);
                    cp.drawImage(0, 0, small_quad);
                    cp.end();

                    p_idx = _current_scene->selectPalette(context_quad, sel_method, color_variations);
                }
            }
            else
            {
                p_idx = current_palette;
            }
            if (p_idx < 0)
            {
                p_idx = palettes.grayscale_index;
            }
            _current_scene->importChar(QPoint(x / 8, y / 8), p_idx, small_quad);
        }
        pd->setValue(progressCounter);
        _current_scene->canvas()->update();
        qApp->processEvents();
    }
    pd->hide();
    pd->deleteLater();
}

QProgressDialog * MainWindow::createProgress(QWidget * parent, QString text)
{
    QProgressDialog * pd = new QProgressDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    pd->setWindowIcon(QPixmap(":/sonic.zx4.png"));
    pd->setWindowTitle("ZX4 Pixelizer");
    pd->setLabelText(text);
    pd->setRange(0, 0);
    pd->setCancelButton(nullptr);
    pd->show();
    qApp->processEvents();
    return pd;
}
