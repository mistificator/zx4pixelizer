#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QFileInfo>
#include <QDebug>
#include <QMimeData>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <algorithm>
#include <cmath>

void MainWindow::showEvent(QShowEvent *)
{
    updateToolbarWidgetsHeight();
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    ui->palettes->scrollToItem(ui->palettes->currentItem(), QAbstractItemView::PositionAtCenter);
}

bool MainWindow::eventFilter(QObject * o, QEvent * ev)
{
    if (ev->type() == QEvent::KeyPress || ev->type() == QEvent::KeyRelease)
    {
        if (current_sprite)
        {
            current_sprite->updateCanvasCursor();
        }
        if (current_scene)
        {
            current_scene->updateCanvasCursor();
        }
    }
    if (o == ui->preview)
    {
        switch (ev->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        {
            auto * me = dynamic_cast<QMouseEvent *>(ev);
            if (me && me->buttons().testFlag(Qt::LeftButton))
            {
                int side_x = 0;
                if (current_sprite && !current_sprite->image().isNull())
                {
                    int step = qMax(1, qMin(ui->preview->width() / current_sprite->canvasSize().width(),
                                            ui->preview->height() / current_sprite->canvasSize().height()));
                    side_x = step * current_sprite->canvasSize().width();
                }
                else if (current_scene && !current_scene->renderedImage().isNull())
                {
                    QSize sz = current_scene->renderedImage().size().scaled(ui->preview->size(), Qt::KeepAspectRatio);
                    side_x = sz.width();
                }
                if (side_x > 0)
                {
                    preview_split_pos = std::clamp(double(me->position().x()) / side_x, 0.0, 1.0);
                    ui->preview->update();
                }
                return true;
            }
            break;
        }
        case QEvent::Paint:
        {
            if (current_sprite && !current_sprite->image().isNull())
            {
                int step = qMax(1, qMin(ui->preview->width() / current_sprite->canvasSize().width(),
                                        ui->preview->height() / current_sprite->canvasSize().height()));
                int side_x = step * current_sprite->canvasSize().width();
                int side_y = step * current_sprite->canvasSize().height();
                int split_x = std::clamp(int(std::round(preview_split_pos * side_x)), 0, side_x);

                QPainter p(ui->preview);

                // Левая часть ("Было" — исходное изображение спрайта)
                if (split_x > 0)
                {
                    p.save();
                    p.setClipRect(0, 0, split_x, side_y);
                    const QImage & src_frame = current_sprite->sourceImage(current_sprite->currentFrame());
                    if (!src_frame.isNull())
                    {
                        int src_w = side_x;
                        int src_h = side_y;
                        if (src_frame.width() < src_frame.height())
                        {
                            src_w = src_frame.width() * side_x / qMax(1, src_frame.height());
                        }
                        else
                        {
                            src_h = src_frame.height() * side_y / qMax(1, src_frame.width());
                        }
                        QImage transformed_src = applyTransforms(
                            current_sprite->importPars(),
                            src_frame.convertToFormat(QImage::Format_ARGB32));
                        p.drawImage((side_x - src_w) / 2,
                                    (side_y - src_h) / 2,
                                    transformed_src.scaled(qMax(1, src_w), qMax(1, src_h)));
                    }
                    else
                    {
                        QList<QColor> c = current_sprite->colors();
                        QImage pattern(4 * step, 4 * step, QImage::Format_RGB32);
                        QPainter pp(&pattern);
                        QList<int> idx = { 0, 3, 2, 1,
                                          1, 0, 3, 2,
                                          2, 1, 0, 3,
                                          3, 2, 1, 0 };
                        for (int i = 0; i < idx.count(); i++)
                        {
                            pp.fillRect((i % 4) * step, (i / 4) * step, step, step, c[idx[i]]);
                        }
                        p.fillRect(0, 0, side_x, side_y, pattern);
                        current_sprite->drawPixels(&p, step);
                    }
                    p.restore();
                }

                // Правая часть ("Стало" — результирующий спрайт)
                if (split_x < side_x)
                {
                    p.save();
                    p.setClipRect(split_x, 0, side_x - split_x, side_y);
                    QList<QColor> c = current_sprite->colors();
                    QImage pattern(4 * step, 4 * step, QImage::Format_RGB32);
                    QPainter pp(&pattern);
                    QList<int> idx = { 0, 3, 2, 1,
                                      1, 0, 3, 2,
                                      2, 1, 0, 3,
                                      3, 2, 1, 0 };
                    for (int i = 0; i < idx.count(); i++)
                    {
                        pp.fillRect((i % 4) * step, (i / 4) * step, step, step, c[idx[i]]);
                    }
                    p.fillRect(0, 0, side_x, side_y, pattern);
                    current_sprite->drawPixels(&p, step);
                    p.restore();
                }

                // Вертикальный разделитель
                p.setPen(QPen(Qt::black, 3));
                p.drawLine(split_x, 0, split_x, side_y);
                p.setPen(QPen(Qt::white, 1));
                p.drawLine(split_x, 0, split_x, side_y);
            }
            else if (current_scene && !current_scene->renderedImage().isNull())
            {
                QImage after_img = current_scene->renderedImage().scaled(ui->preview->size(), Qt::KeepAspectRatio);
                int side_x = after_img.width();
                int side_y = after_img.height();
                int split_x = std::clamp(int(std::round(preview_split_pos * side_x)), 0, side_x);

                QPainter p(ui->preview);

                // Левая часть ("Было" — исходное изображение сцены)
                if (split_x > 0 && !current_scene->sourceImage().isNull())
                {
                    QImage before_img = applyTransforms(current_scene->importPars(), current_scene->sourceImage())
                    .scaled(ui->preview->size(), Qt::KeepAspectRatio);
                    p.save();
                    p.setClipRect(0, 0, split_x, side_y);
                    p.drawImage(0, 0, before_img);
                    p.restore();
                }

                // Правая часть ("Стало" — сконвертированная сцена)
                if (split_x < side_x)
                {
                    p.save();
                    p.setClipRect(split_x, 0, side_x - split_x, side_y);
                    p.drawImage(0, 0, after_img);
                    p.restore();
                }

                // Вертикальный разделитель
                p.setPen(QPen(Qt::black, 3));
                p.drawLine(split_x, 0, split_x, side_y);
                p.setPen(QPen(Qt::white, 1));
                p.drawLine(split_x, 0, split_x, side_y);
            }
            break;
        }
        default:
            break;
        }
    }
    else if (o == ui->mdiArea)
    {
        switch (ev->type())
        {
        case QEvent::DragEnter:
        {
            auto *de = dynamic_cast<QDragEnterEvent *>(ev);
            if (de && de->mimeData() && de->mimeData()->hasUrls())
            {
                de->acceptProposedAction();
                return true;
            }
            break;
        }
        case QEvent::Drop:
        {
            auto *de = dynamic_cast<QDropEvent *>(ev);
            if (de && de->mimeData() && de->mimeData()->hasUrls())
            {
                for (const auto & url : de->mimeData()->urls())
                {
                    if (url.isLocalFile())
                    {
                        QString filename = url.toLocalFile();
                        QFileInfo fi(filename);
                        if (fi.completeSuffix().compare("zx4.png", Qt::CaseInsensitive) == 0 ||
                            fi.completeSuffix().compare("zx4.apng", Qt::CaseInsensitive) == 0)
                        {
                            QImageReader rdr(filename);
                            QList<QImage> list;
                            QImage frame;
                            while (!(frame = rdr.read()).isNull())
                            {
                                list << frame;
                            }
                            if (list.count() == 1 && list[0].text("ZX4_Frames").toInt() > 1)
                            {
                                QImage sheet = list[0];
                                int frames = sheet.text("ZX4_Frames").toInt();
                                int frame_h = sheet.height() / frames;
                                list.clear();
                                for (int k = 0; k < frames; k++)
                                {
                                    QImage sub = sheet.copy(0, k * frame_h, sheet.width(), frame_h);
                                    sub.setColorTable(sheet.colorTable());
                                    list << sub;
                                }
                            }
                            openSprite(list, filename);
                        }
                        else if (fi.completeSuffix().compare("zx4", Qt::CaseInsensitive) == 0)
                        {
                            importSpriteZx4Format(filename);
                        }
                        else
                        {
                            importScene(QImage(filename), filename);
                        }
                    }
                    else
                    {
                        auto net = new QNetworkAccessManager(this);
                        auto reply = net->get(QNetworkRequest(url));
                        connect(reply, &QNetworkReply::finished, this, [=]()
                                {
                                    importScene(QImageReader(reply).read(), url.fileName());
                                    reply->deleteLater();
                                    net->deleteLater();
                                });
                        connect(reply, &QNetworkReply::errorOccurred, this, [=](QNetworkReply::NetworkError err)
                                {
                                    qDebug() << err << reply->errorString();
                                });
                    }
                }
                de->acceptProposedAction();
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
    return QMainWindow::eventFilter(o, ev);
}
