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
    ui->palettes->setCurrentRow(327 - 1);
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
        qApp->processEvents();
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

                            color_palette_table[color_num] << z[0] << z[1] << z[2] << z[3];

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
        qApp->processEvents();
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
        qApp->processEvents();
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
        qApp->processEvents();
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
    QPixmap px(QSize(32, 32));
    px.fill(list[0]);
    ui->actionColor_00->setIcon(px);
    px.fill(list[1]);
    ui->actionColor_01->setIcon(px);
    px.fill(list[2]);
    ui->actionColor_10->setIcon(px);
    px.fill(list[3]);
    ui->actionColor_11->setIcon(px);

    if (current_sprite)
    {
        current_sprite->setPaletteIndex(ui->palettes->currentRow());
        current_sprite->setColors(list);
    }
}

void MainWindow::on_actionColor_00_toggled(bool)
{
    if (current_sprite)
    {
        current_sprite->setCurrentColorIndex(0);
    }
}

void MainWindow::on_actionColor_01_toggled(bool)
{
    if (current_sprite)
    {
        current_sprite->setCurrentColorIndex(1);
    }
}

void MainWindow::on_actionColor_10_toggled(bool)
{
    if (current_sprite)
    {
        current_sprite->setCurrentColorIndex(2);
    }
}

void MainWindow::on_actionColor_11_toggled(bool)
{
    if (current_sprite)
    {
        current_sprite->setCurrentColorIndex(3);
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
            QColor px;

            QColor old_px_rgb = out.pixelColor(ix, iy);
            QColor new_px_rgb = img.pixelColor(x, y);

            QColor old_px_hsl = old_px_rgb.toHsl();
            QColor new_px_hsl = new_px_rgb.toHsl();

            if (new_px_hsl.hslHue() == -1 || old_px_hsl.hslHue() == -1)
            {
                px = new_px_rgb;
            }
            else
            {
                px = new_px_hsl.hslSaturation() > old_px_hsl.hslSaturation() ? new_px_hsl : old_px_hsl;

                px.setHsl(
                            px.hslHue(),
                            px.hslSaturation(),
                            px.lightness(),
                            qMax(new_px_hsl.alpha(), old_px_hsl.alpha()));
            }

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
    if (!current_sprite)
    {
        return QImage();
    }

    ui->palettes->setCurrentItem(ui->palettes->item(palette_number));

    img = scaleImage(img, current_sprite->canvasSize().width(), current_sprite->canvasSize().height());

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

void MainWindow::on_palettes_itemClicked(QListWidgetItem *)
{
    on_palettes_itemSelectionChanged();
}

void MainWindow::on_actionNew_triggered()
{
    current_sprite = new SpriteWindow();
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
    if (!current_sprite)
    {
        return;
    }
    int p_idx = ui->palettes->currentRow();
    if (ui->wizardPaletteBox->currentIndex() == 0)
    {
        QMultiMap<double, int> dist = selectPalette(current_sprite->sourceImage());
        ui->paletteNumberBox->setRange(1, dist.count() + 1);
        p_idx = * (dist.begin() + (ui->paletteNumberBox->value() - 1));
    }
    current_sprite->setPaletteIndex(p_idx);
    current_sprite->drawImage(importImage(current_sprite->sourceImage(),  p_idx, ui->alphaBox->isChecked()));
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
        return;
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
    QImage src_imp_image = QImage(filename).convertToFormat(QImage::Format_ARGB32);
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
    QList<ZxColorEnum> z = color_palette_table[p_idx];

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
        return;
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
    if (palettes_by_4colors.contains(QPair<QColor, QColor>(colors[0], colors[1])) && palettes_by_4colors[QPair<QColor, QColor>(colors[0], colors[1])].contains(QPair<QColor, QColor>(colors[2], colors[3])))
    {
        QList<int> list = palettes_by_4colors[QPair<QColor, QColor>(colors[0], colors[1])][QPair<QColor, QColor>(colors[2], colors[3])];
        if (!list.isEmpty())
        {
            p_idx = list.first();
        }
    }
    current_sprite->setPaletteIndex(p_idx);
    ui->palettes->setCurrentRow(p_idx);

    current_sprite->setFileName(filename);
    current_sprite->clearCanvas(QSize(qMin(src_imp_image.width(), 255), qMin(src_imp_image.height(), 192)));
    current_sprite->setSourceImage(src_imp_image.convertToFormat(QImage::Format_ARGB32));
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
    foreach(QColor color, colors())
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
        current_scene = new SceneWindow();
        ui->mdiArea->addSubWindow(current_scene);
        current_scene->show();
    }
    current_scene->setFileName(filename);

    img = scaleImage(img, qMin(((img.width() + 7) / 8) * 8, 256), qMin(((img.height() + 7) / 8) * 8, 192)).convertToFormat(QImage::Format_Indexed8);

    Zx4Image scene;
    scene.resize(((img.width() + 7) / 8) * 8, ((img.height() + 7) / 8) * 8);
    QMap<int, QMap<int, int>> scene_attr;

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
                small_quad = small_quad.convertToFormat(QImage::Format_RGB32).convertToFormat(QImage::Format_Indexed8, color_table);
                p.drawImage(QRect(x, y, 8, 8), small_quad);

                scene_attr[x / 8][y / 8] = p_idx;
                for (int iy = 0; iy < 8; iy++)
                {
                    for (int ix = 0; ix < 8; ix++)
                    {
                        int idx = small_quad.pixelIndex(ix, iy);
                        if (idx >= 0 && idx < 4)
                        {
                            scene.color_layer0[iy + y][ix + x] = idx == 0 || idx == 1 ? 0 : 1;
                            scene.color_layer1[iy + y][ix + x] = idx == 0 || idx == 2 ? 0 : 1;
                            scene.alpha_layer[iy + y][ix + x] = 1;
                        }
                        else
                        {
                            qDebug() << "wrong color index" << idx;
                        }
                    }
                }
            }
        }
    }

    current_scene->setImage(scene);
    current_scene->setAttr(scene_attr);
    current_scene->setSourceImage(img);
    current_scene->canvas()->setPixmap(QPixmap::fromImage(out));

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

    QString filename = QFileDialog::getSaveFileName(this, "Save file", path, "ZX Spectrum screen (*.scr0)");
    if (filename.isEmpty())
    {
        return;
    }
    if (QFileInfo(filename).suffix().isEmpty())
    {
        filename += ".scr0";
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
    for (int y = 0; y < scene.color_layer0.count() / 8; y++)
    {
        for (int x = 0; x < scene.color_layer0[y].count() / 8; x++)
        {
            int p_idx = scene_attr[x][y];
            QList<ZxColorEnum> z = color_palette_table[p_idx];
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
    QMessageBox::about(this, "ZX4 Pixelizer", "ZX4 Pixelizer\nImages & sprites manipulation tool\nfor ZX Spectrum 128K\n\n© 2022 ZXLDR Custom");
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


