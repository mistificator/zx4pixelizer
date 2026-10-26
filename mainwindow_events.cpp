#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
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
        auto * ke = static_cast<QKeyEvent *>(ev);
        if (ke->key() == Qt::Key_Control || ke->key() == Qt::Key_Shift)
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
                        const QSize target_sz = src_frame.size().scaled(side_x, side_y, Qt::KeepAspectRatio);
                        const int src_w = qMax(1, target_sz.width());
                        const int src_h = qMax(1, target_sz.height());

                        QImage scaled_src = src_frame.convertToFormat(QImage::Format_ARGB32)
                                                .scaled(src_w, src_h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                        QImage transformed_src = applyTransforms(*current_sprite, scaled_src);
                        p.drawImage((side_x - src_w) / 2,
                                    (side_y - src_h) / 2,
                                    transformed_src);
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

                if (split_x > 0 && !current_scene->sourceImage().isNull())
                {
                    QImage before_img = applyTransforms(*current_scene, current_scene->sourceImage())
                    .scaled(ui->preview->size(), Qt::KeepAspectRatio);
                    p.save();
                    p.setClipRect(0, 0, split_x, side_y);
                    p.drawImage(0, 0, before_img);
                    p.restore();
                }

                if (split_x < side_x)
                {
                    p.save();
                    p.setClipRect(split_x, 0, side_x - split_x, side_y);
                    p.drawImage(0, 0, after_img);
                    p.restore();
                }

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
    else if (o == ui->mdiArea || o == ui->mdiArea->viewport())
    {
        switch (ev->type())
        {
        case QEvent::DragEnter:
        case QEvent::DragMove:
        {
            auto *de = dynamic_cast<QDragMoveEvent *>(ev);
            if (de && de->mimeData() && (de->mimeData()->hasUrls() || de->mimeData()->hasImage()))
            {
                de->acceptProposedAction();
                return true;
            }
            break;
        }
        case QEvent::Drop:
        {
            auto *de = dynamic_cast<QDropEvent *>(ev);
            if (!de || !de->mimeData())
            {
                break;
            }

            const QMimeData * mime = de->mimeData();
            QImage fallback_img = mime->hasImage() ? qvariant_cast<QImage>(mime->imageData()) : QImage();

            if (mime->hasUrls() && !mime->urls().isEmpty())
            {
                for (const auto & url : mime->urls())
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
                    else if (url.scheme().compare("data", Qt::CaseInsensitive) == 0)
                    {
                        const QString raw = url.toString();
                        const int comma = raw.indexOf(',');
                        QImage img;
                        if (comma > 0)
                        {
                            const QByteArray payload = raw.mid(comma + 1).toLatin1();
                            const QByteArray decoded = raw.left(comma).contains("base64", Qt::CaseInsensitive)
                                                           ? QByteArray::fromBase64(payload)
                                                           : QByteArray::fromPercentEncoding(payload);
                            img = QImage::fromData(decoded);
                        }
                        if (img.isNull() && !fallback_img.isNull())
                        {
                            img = fallback_img;
                        }
                        if (!img.isNull())
                        {
                            importScene(img, "data_url");
                        }
                    }
                    else
                    {
                        auto net = new QNetworkAccessManager(this);
                        QNetworkRequest req(url);
                        req.setHeader(QNetworkRequest::UserAgentHeader,
                                      "Mozilla/5.0 (Windows NT 10.0; Win64; x64) ZX4Pixelizer");
                        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                         QNetworkRequest::NoLessSafeRedirectPolicy);

                        auto reply = net->get(req);
                        connect(reply, &QNetworkReply::finished, this, [this, reply, net, url, fallback_img]()
                                {
                                    QByteArray data = reply->readAll();
                                    QImage img = QImage::fromData(data);
                                    if (img.isNull() && !fallback_img.isNull())
                                    {
                                        img = fallback_img;
                                    }
                                    QString name = url.fileName();
                                    if (name.isEmpty())
                                    {
                                        name = "url_image";
                                    }
                                    importScene(img, name);
                                    reply->deleteLater();
                                    net->deleteLater();
                                });
                        connect(reply, &QNetworkReply::errorOccurred, this, [](QNetworkReply::NetworkError err)
                                {
                                    qDebug() << "Network drop error:" << err;
                                });
                    }
                }
                de->acceptProposedAction();
                return true;
            }
            else if (!fallback_img.isNull())
            {
                importScene(fallback_img, "dropped_image");
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
