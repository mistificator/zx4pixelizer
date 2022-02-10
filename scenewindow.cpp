#include <QFileInfo>
#include "scenewindow.h"
#include "ui_scenewindow.h"

SceneWindow::SceneWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SceneWindow)
{
    ui->setupUi(this);
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
    src_imp_img = i;
}

const QImage & SceneWindow::sourceImage() const
{
    return src_imp_img;
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
}

const Zx4Image & SceneWindow::image() const
{
    return scene;
}

void SceneWindow::setAttr(QMap<int, QMap<int, int>> a)
{
    scene_attr = a;
}

QMap<int, QMap<int, int>> SceneWindow::attr() const
{
    return scene_attr;
}
