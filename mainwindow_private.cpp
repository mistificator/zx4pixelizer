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

#include <omp.h>


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

Zx4Palette::SelectPaletteMethod MainWindow::selectPaletteMethod() const
{
    return static_cast<Zx4Palette::SelectPaletteMethod>(ui->colorPickBox->currentIndex());
}

QImage MainWindow::applyTransforms(const Zx4ImportPars & p, const QImage & src, QColor bg_color) const
{
    int mirror = p.horizontal_mirror ? -1 : 1;
    QImage img(src.size(), src.format());
    img.fill(bg_color);

    QPainter pmx(& img);

    QTransform mx;
    mx.translate(p.x_shift, p.y_shift);
    mx.translate(img.width() / 2, img.height() / 2);
    mx.rotate(p.rotate);
    mx.translate(-img.width() / 2, -img.height() / 2);
    mx.scale(p.x_scale * mirror, p.y_scale);

    pmx.setTransform(mx);

    QImage color_shifted = src;
    if (p.hue_shift != 0 || p.saturation_shift != 0 || p.lightness_shift != 0)
    {
        for (int y = 0; y < color_shifted.height(); ++y)
        {
            for (int x = 0; x < color_shifted.width(); ++x)
            {
                QColor c = color_shifted.pixelColor(x, y).toHsl();
                qreal h, s, l;
                c.getHslF(& h, & s, & l);
                h += p.hue_shift;
                while (h < 0) h += 1.0;
                while (h > 1.0) h -= 1.0;
                s = qMax(0.0, qMin(1.0, s + p.saturation_shift));
                l = qMax(0.0, qMin(1.0, l + p.lightness_shift));
                c.setHslF(h, s, l);
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

//    if (current_scene == 0)
    {
        current_scene = new SceneWindow(palettes);
        importParsFromObjectToWidget(* current_scene);
        ui->mdiArea->addSubWindow(current_scene);
        current_scene->show();
        connect(current_scene, SIGNAL(charSelected(int)), this, SLOT(selectPalette(int)), Qt::QueuedConnection);
        connect(current_scene, SIGNAL(previewUpdateIsNeeded()), ui->preview, SLOT(update()));
        connect(current_scene, SIGNAL(previewUpdateIsNeeded()), ui->sourceImage, SLOT(update()));
    }

    current_scene->setFileName(filename);

    img = img.scaled(qMin(((img.width() + 7) / 8) * 8, 256), qMin(((img.height() + 7) / 8) * 8, 192), Qt::KeepAspectRatio);

    QImage dst_img(256, 192, img.format());
    dst_img.fill(0);
    QPainter p(& dst_img);
    p.drawImage((dst_img.width() - img.width()) / 2, (dst_img.height() - img.height()) / 2, img);
    current_scene->Zx4BaseObject::setSourceImage(dst_img);

    reimport(current_scene);
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
}

//#define USE_RLE8

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
}

void MainWindow::importSpriteZx4Format(QString filename)
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

    ui->actionNew->trigger();

    selectPalette(p_idx);
    current_sprite->setPaletteIndex(p_idx);
    const int width = zx4[2], height = zx4[3];
    current_sprite->setCanvasSize(QSize(width, height));
    const int frames_count = zx4[4];

    QByteArray zx4_data = zx4.mid(5);
#ifndef USE_RLE8
    QByteArray zx4_lz4 = zx4_data;
    zx4.resize(3 * width * height);

    int isize = zx4_lz4.count();
    int osize = ::LZ4_uncompress_unknownOutputSize(zx4_lz4.constData(), zx4.data(), & isize, zx4.count());
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
        zx4_image->resize(width, height);
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
                for (int bit = 0; bit < 8 && (8 * x) + bit < width; bit++)
                {
                    alpha[(8 * x) + (7 - bit)][y] = static_cast<Zx4Image::Bit>((zx4[alpha_bit / 8] >> (alpha_bit % 8)) & 0x01);
                    ++alpha_bit;
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
                for (int bit = 0; bit < 8 && (8 * x) + bit < width; bit++)
                {
                    char alpha_value = alpha[(8 * x) + (7 - bit)][y];
                    if (alpha_value)
                    {
                        layer0[(8 * x) + (7 - bit)][y] = static_cast<Zx4Image::Bit>((zx4_color0[color_bit / 8] >> (color_bit % 8)) & 0x01);
                        layer1[(8 * x) + (7 - bit)][y] = static_cast<Zx4Image::Bit>((zx4_color1[color_bit / 8] >> (color_bit % 8)) & 0x01);
                        ++color_bit;
                    }
                }
            }
        }
    }
    current_sprite->update();
}

void MainWindow::exportSpriteZx4Format(QString filename)
{
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

    const int width = current_sprite->canvasSize().width();
    const int height = current_sprite->canvasSize().height();
    const int layer_sz = ((width * height + 7) / 8) * current_sprite->framesCount();
    QByteArray zx4;
    // 4-color palette
    zx4 += static_cast<char>(z[1] & 0x07) | ((z[0] & 0x0F) << 3);
    zx4 += static_cast<char>(z[3] & 0x07) | ((z[2] & 0x0F) << 3);
    // width & height
    zx4 += static_cast<char>(width);
    zx4 += static_cast<char>(height);
    // frames count
    zx4 += static_cast<char>(current_sprite->framesCount());

    int alpha_bit = 0, color_bit = 0;
    QByteArray zx4_alpha(layer_sz, 0), zx4_color0(layer_sz, 0), zx4_color1(layer_sz, 0);
    QVector<Zx4Image::Bit> alpha_t;
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
                for (int bit = 0; bit < 8 && (8 * x) + bit < width; bit++)
                {
                    auto alpha_value = alpha[(8 * x) + (7 - bit)][y];
                    alpha_t << alpha_value;
                    zx4_alpha[alpha_bit / 8] =         static_cast<char>(zx4_alpha[alpha_bit / 8] | (static_cast<int>(alpha_value) << (alpha_bit % 8)));
                    if (alpha_value)
                    {
                        zx4_color0[color_bit / 8] =    static_cast<char>(zx4_color0[color_bit / 8] | (static_cast<int>(layer0[(8 * x) + (7 - bit)][y]) << (color_bit % 8)));
                        zx4_color1[color_bit / 8] =    static_cast<char>(zx4_color1[color_bit / 8] | (static_cast<int>(layer1[(8 * x) + (7 - bit)][y]) << (color_bit % 8)));
                        ++color_bit;
                    }
                    ++alpha_bit;
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
    f.open(QFile::WriteOnly);
    if (f.isOpen())
    {
        f.write(zx4);
    }
    f.close();
}

void MainWindow::exportScreens(const Zx4ImageWithAttributes & image, QString filename_scr0, QString filename_scr1)
{
    if (QFileInfo(filename_scr0).suffix().compare("scr0", Qt::CaseInsensitive) != 0)
    {
        filename_scr0 = filename_scr0.replace("." + QFileInfo(filename_scr0).suffix(), ".scr0");
    }
    if (filename_scr1.isEmpty())
    {
        filename_scr1 = filename_scr0;
    }
    if (QFileInfo(filename_scr1).suffix().compare("scr1", Qt::CaseInsensitive) != 0)
    {
        filename_scr1 = filename_scr1.replace("." + QFileInfo(filename_scr1).suffix(), ".scr1");
    }

    auto screens = image.makeScreens(palettes);

    QFile f0(filename_scr0), f1(filename_scr1);
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

void MainWindow::exportTape(const Zx4ImageWithAttributes & image, QString filename)
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

    auto screens = image.makeScreens(palettes);
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
}

void MainWindow::importParsFromWidgetToObject(Zx4BaseObject & b)
{
    Zx4ImportPars p = b.importPars();
    p.alpha = ui->alphaBox->isChecked();
    p.autopalette = ui->wizardPaletteBox->currentIndex() == 0;
    p.color_pick = static_cast<Zx4Palette::SelectPaletteMethod>(ui->colorPickBox->currentIndex());
    p.horizontal_mirror = ui->mirrored->isChecked();
    p.multipass = static_cast<Zx4ImportPars::PassMethod>(ui->multipass->currentIndex());
    p.palette_index_max = ui->paletteNumberBox->maximum();
    p.palette_index = ui->paletteNumberBox->value();
    p.rotate = ui->rotate->value();
//    p.vertical_mirror = false;
    p.x_scale = ui->xScale->value();
    p.x_shift = ui->xShift->value();
    p.y_scale = ui->yScale->value();
    p.y_shift = ui->yShift->value();
    p.hue_shift = ui->hueShift->value();
    p.saturation_shift = ui->saturationShift->value();
    p.lightness_shift = ui->lightnessShift->value();
    b.setImportPars(p);
}

void MainWindow::importParsFromObjectToWidget(const Zx4BaseObject & b)
{
    Zx4ImportPars p = b.importPars();
    ui->alphaBox->setChecked(p.alpha);
    ui->wizardPaletteBox->setCurrentIndex(p.autopalette ? 0 : 1);
    ui->colorPickBox->setCurrentIndex(p.color_pick);
    ui->mirrored->setChecked(p.horizontal_mirror);
    ui->multipass->setCurrentIndex(p.multipass);
    ui->paletteNumberBox->setMaximum(p.palette_index_max);
    ui->paletteNumberBox->setValue(p.palette_index);
    ui->rotate->setValue(p.rotate);
//    p.vertical_mirror;
    ui->xScale->setValue(p.x_scale);
    ui->xShift->setValue(p.x_shift);
    ui->yScale->setValue(p.y_scale);
    ui->yShift->setValue(p.y_shift);
    ui->hueShift->setValue(p.hue_shift);
    ui->saturationShift->setValue(p.saturation_shift);
    ui->lightnessShift->setValue(p.lightness_shift);
}

void MainWindow::reimport(SpriteWindow * _current_sprite, int p_idx)
{
    if (!_current_sprite)
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
                ui->paletteNumberBox->setRange(1, dist.count());
                ui->paletteNumberBox->setToolTip(QString("%1..%2").arg(1).arg(dist.count()));
                p_idx = * (dist.begin() + (ui->paletteNumberBox->value() - 1));
            }
        }
    }
    _current_sprite->setPaletteIndex(p_idx);

    int current_frame = _current_sprite->currentFrame();
    for (int k = 0; k < _current_sprite->framesCount(); k++)
    {
        _current_sprite->setCurrentFrame(k);
        _current_sprite->drawImage(importImage(applyTransforms(_current_sprite->importPars(), _current_sprite->sourceImage(k).convertToFormat(QImage::Format_ARGB32)), p_idx, ui->alphaBox->isChecked()));
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

    QProgressDialog * pd = createProgress(this, "Importing scene\n" + ui->colorPickBox->currentText());

    _current_scene->transformedImage() = applyTransforms(_current_scene->importPars(), _current_scene->sourceImage());
    QImage img8 = _current_scene->transformedImage().convertToFormat(QImage::Format_Indexed8);
    pd->setRange(0, (img8.width() / 8) * (img8.height() / 8));

#if 0
    auto img8_dist = colorDistribution(img8, false);
    qDebug() << img8_dist.count() << img8_dist;

    auto img_dist = colorDistribution(img, false);
    qDebug() << img_dist.count() << img_dist;
#endif

    QImage out(img8.width(), img8.height(), QImage::Format_RGB32);

    foreach(QMdiSubWindow * msw, ui->mdiArea->subWindowList())
    {
        if (msw->widget() == _current_scene)
        {
            QPoint pos = _current_scene->canvas()->mapTo(msw, QPoint(0, 0));
            msw->resize(out.width() * 4 + pos.x(), out.height() * 4 + pos.y());
            break;
        }
    }

    QPainter p(& out);
    int createProgress = 0;
    const auto sel_method = selectPaletteMethod();
    int multipass_offset = 2;
    int multipass_step = 2;
    const Zx4ImportPars::PassMethod multipass = static_cast<Zx4ImportPars::PassMethod>(ui->multipass->currentIndex());
    switch (multipass)
    {
    case Zx4ImportPars::Multipass3x3x1:
        multipass_offset = 1;
        multipass_step = 1;
        break;
    case Zx4ImportPars::Multipass5x5x1:
        multipass_offset = 2;
        multipass_step = 1;
        break;
    case Zx4ImportPars::Multipass5x5x2:
        multipass_offset = 2;
        multipass_step = 2;
        break;
    case Zx4ImportPars::Multipass7x7x1:
        multipass_offset = 3;
        multipass_step = 1;
        break;
    case Zx4ImportPars::Multipass7x7x3:
        multipass_offset = 3;
        multipass_step = 3;
        break;
    default:
        break;
    }

    const int width = img8.width();
    const int height = img8.height();

    #pragma omp parallel for num_threads(std::min(omp_get_num_procs() - 1, height / 8))
    for (int y = 0; y < height; y += 8)
    {
        for (int x = 0; x < width; x += 8)
        {
            #pragma omp atomic
                createProgress++;

            int p_idx = -1;

            QImage small_quad = img8.copy(QRect(x, y, 8, 8));
            if (multipass == Zx4ImportPars::SinglePass)
            {
                p_idx = _current_scene->selectPalette(small_quad, sel_method);
            }
            else
            {
                QMap<int, int> map;
                for (int y1 = -multipass_offset; y1 <= multipass_offset; y1 += multipass_step)
                {
                    for (int x1 = -multipass_offset; x1 <= multipass_offset; x1 += multipass_step)
                    {
                        map[_current_scene->selectPalette(
                            img8.copy(QRect(qMax(x + x1, 0), qMax(y + y1, 0), qMin(8, width - (x + x1)), qMin(8, height - (y + y1)))),
                            sel_method)]++;
                    }
                }
                int map_max = 0;
                for (auto i = map.begin(); i != map.end(); ++i)
                {
                    if (i.value() > map_max)
                    {
                        map_max = i.value();
                        p_idx = i.key();
                    }
                }
            }
            if (p_idx < 0)
            {
                p_idx = palettes.grayscale_index;
            }

            _current_scene->importChar(QPoint(x / 8, y / 8), p_idx, small_quad);

            if (multipass != Zx4ImportPars::SinglePass)
            {
                QMetaObject::invokeMethod(pd, "setValue", Qt::QueuedConnection, Q_ARG(int, createProgress));
                QMetaObject::invokeMethod(_current_scene->canvas(), "update", Qt::QueuedConnection);
                #pragma omp critical
                    qApp->processEvents();
            }
        }
        if (multipass == Zx4ImportPars::SinglePass)
        {
            QMetaObject::invokeMethod(pd, "setValue", Qt::QueuedConnection, Q_ARG(int, createProgress));
            QMetaObject::invokeMethod(_current_scene->canvas(), "update", Qt::QueuedConnection);
            #pragma omp critical
                qApp->processEvents();
        }
    }

    pd->hide();
    pd->deleteLater();

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

QProgressDialog * MainWindow::createProgress(QWidget * parent, QString text)
{
    QProgressDialog & pd = * new QProgressDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    pd.setWindowIcon(QPixmap(":/sonic.zx4.png"));
    pd.setWindowTitle("ZX4 Pixelizer");
    pd.setLabelText(text);
    pd.setRange(0, 0);
    pd.setCancelButton(nullptr);
    pd.show();
    qApp->processEvents();
    return & pd;
}
