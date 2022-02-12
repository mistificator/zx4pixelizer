#include <QFileInfo>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include "scenewindow.h"
#include "ui_scenewindow.h"

SceneWindow::SceneWindow(const Zx4Palette & palettes, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SceneWindow),
    palettes(palettes)
{
    ui->setupUi(this);
    ui->canvas->installEventFilter(this);

    setFileName(QString());
}

SceneWindow::~SceneWindow()
{
    delete ui;
}

void SceneWindow::setFileName(QString s)
{
    file_name = s;
    setWindowTitle((!file_name.isEmpty() ? QFileInfo(file_name).fileName() : "untitled") + " - Scene");
}

QString SceneWindow::fileName() const
{
    return file_name;
}

void SceneWindow::setSourceImage(const QImage & i)
{
    scene.resize(i.width(), i.height());
    src_imp_img = i;
    ui->canvas->update();
}

const QImage & SceneWindow::sourceImage() const
{
    return src_imp_img;
}

const QImage & SceneWindow::renderedImage() const
{
    return rdr_img;
}

QSize SceneWindow::sizeHint() const
{
    return QSize(640, 640);
}

QLabel * SceneWindow::canvas() const
{
    return ui->canvas;
}

void SceneWindow::setImage(const Zx4Image & i)
{
    scene = i;
    ui->canvas->update();
}

const Zx4Image & SceneWindow::image() const
{
    return scene;
}

void SceneWindow::setAttr(QMap<int, QMap<int, int>> a)
{
    scene_attr = a;
    ui->canvas->update();
}

QMap<int, QMap<int, int>> SceneWindow::attr() const
{
    return scene_attr;
}


QPoint SceneWindow::currentChar() const
{
    return current_char;
}

void SceneWindow::setAttr(QPoint p, int p_idx)
{
    scene_attr[p.x()][p.y()] = p_idx;
}

void SceneWindow::setAttr(int p_idx)
{
    setAttr(current_char, p_idx);
}

bool SceneWindow::eventFilter(QObject * o, QEvent * ev)
{
    if (o == ui->canvas)
    {
        switch (ev->type())
        {
            case QEvent::Paint:
                drawGrid();
            break;
            case QEvent::MouseButtonPress:
            case QEvent::MouseMove:
            {
                QMouseEvent * me = dynamic_cast<QMouseEvent *>(ev);
                double step = qMin(1.0 * grid_x / src_imp_img.width(), 1.0 * grid_y / src_imp_img.height());

                QPoint pt = me->pos();
                int x = (pt.x() - 1) / step;
                int y = (pt.y() - 1) / step;

                if (me->buttons().testFlag(Qt::LeftButton))
                {
                    if (me->modifiers().testFlag(Qt::ShiftModifier))
                    {
                        importChar(
                            QPoint(x / 8, y / 8),
                            scene_attr[x / 8][y / 8] = scene_attr[current_char.x()][current_char.y()],
                            src_imp_img.convertToFormat(QImage::Format_Indexed8).copy((x / 8) * 8, (y / 8) * 8, 8, 8));
                    }
                    current_char = QPoint(x / 8, y / 8);
                    if (me->modifiers().testFlag(Qt::ControlModifier))
                    {
                        emit charSelected(scene_attr[current_char.x()][current_char.y()]);
                    }
                    QMetaObject::invokeMethod(ui->canvas, "update", Qt::QueuedConnection);
                }
            }
            default:
            break;
        }
    }
    return QWidget::eventFilter(o, ev);
}

void SceneWindow::drawGrid()
{
    scene.resize(src_imp_img.width(), src_imp_img.height());
    rdr_img = QImage(src_imp_img.width(), src_imp_img.height(), QImage::Format_RGB32);
    for (int y = 0; y < rdr_img.height(); y++)
    {
        for (int x = 0; x < rdr_img.width(); x++)
        {
            int p_idx = scene_attr[x / 8][y / 8];
            auto c = palettes.palettes[p_idx];

            char l0 = scene.color_layer0[y][x];
            char l1 = scene.color_layer1[y][x];
            rdr_img.setPixelColor(x, y, !l0 && !l1 ? c[0] : l0 && !l1 ? c[2] : !l0 && l1 ? c[1] : c[3]);
        }
    }
    QImage out = rdr_img.scaled(ui->canvas->size(), Qt::KeepAspectRatio);
    grid_x = out.width();
    grid_y = out.height();

    QPainter p(ui->canvas);
    p.fillRect(ui->canvas->rect(), Qt::black);
    p.drawImage(0, 0, out);

    QMetaObject::invokeMethod(this, "previewUpdateIsNeeded", Qt::QueuedConnection);
}

void SceneWindow::importChar(QPoint char_pos, int p_idx, QImage small_quad)
{
    if (p_idx < 0 || scene.isNull())
    {
        return;
    }

    QVector<QRgb> color_table;
    foreach(QColor c, palettes.palettes[p_idx])
    {
        color_table << c.rgb();
    }
    small_quad = small_quad.convertToFormat(QImage::Format_RGB32).convertToFormat(QImage::Format_Indexed8, color_table);

    scene_attr[char_pos.x()][char_pos.y()] = p_idx;
    for (int iy = 0; iy < 8; iy++)
    {
        int y = iy + char_pos.y() * 8;
        if (y < src_imp_img.height())
        for (int ix = 0; ix < 8; ix++)
        {
            int x = ix + char_pos.x() * 8;
            if (x < src_imp_img.width())
            {
                int idx = small_quad.pixelIndex(ix, iy);
                if (idx >= 0 && idx < 4)
                {
                    scene.color_layer0[y][x] = idx == 0 || idx == 1 ? 0 : 1;
                    scene.color_layer1[y][x] = idx == 0 || idx == 2 ? 0 : 1;
                    scene.alpha_layer[y][x] = 1;
                }
                else
                {
                    qDebug() << "wrong color index" << idx;
                }
            }
        }
    }
}
