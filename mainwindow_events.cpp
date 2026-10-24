#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QResizeEvent>
#include <QFileInfo>
#include <QDebug>
#include <QMimeData>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

void MainWindow::showEvent(QShowEvent *)
{
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    ui->palettes->scrollToItem(ui->palettes->currentItem(), QAbstractItemView::PositionAtCenter);
}

bool MainWindow::eventFilter(QObject * o, QEvent * ev)
{
    if (o == ui->preview)
    {
        switch (ev->type())
        {
        case QEvent::Paint:
            if (current_sprite && !current_sprite->image().isNull())
            {
                int step = qMax(1, qMin(ui->preview->width() / current_sprite->canvasSize().width(),
                                        ui->preview->height() / current_sprite->canvasSize().height()));
                int side_x = step * current_sprite->canvasSize().width();
                int side_y = step * current_sprite->canvasSize().height();

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
                QPainter p(ui->preview);
                p.fillRect(0, 0, side_x, side_y, pattern);
                current_sprite->drawPixels(&p, step);
            }
            else if (current_scene && !current_scene->renderedImage().isNull())
            {
                QPainter p(ui->preview);
                p.drawImage(0, 0, current_scene->renderedImage().scaled(ui->preview->size(), Qt::KeepAspectRatio));
            }
            break;

        default:
            break;
        }
    }
    else if (o == ui->sourceImage)
    {
        switch (ev->type())
        {
        case QEvent::Paint:
            if (current_sprite && !current_sprite->sourceImage().isNull())
            {
                int step = qMax(1, qMin(ui->sourceImage->width() / current_sprite->canvasSize().width(),
                                        ui->sourceImage->height() / current_sprite->canvasSize().height()));
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
                p.drawImage((step * current_sprite->canvasSize().width() - side_x) / 2,
                            (step * current_sprite->canvasSize().height() - side_y) / 2,
                            current_sprite->sourceImage(current_sprite->currentFrame()).scaled(qMax(1, side_x), qMax(1, side_y)));
            }
            else if (current_scene && !current_scene->sourceImage().isNull())
            {
                QPainter p(ui->sourceImage);
                p.drawImage(0, 0, applyTransforms(current_scene->importPars(), current_scene->sourceImage()).scaled(ui->sourceImage->size(), Qt::KeepAspectRatio));
            }
            break;

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
