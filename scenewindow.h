#ifndef SCENEWINDOW_H
#define SCENEWINDOW_H

#include <QWidget>
#include <QMap>
#include <QLabel>
#include "zx4pixelizer.h"

namespace Ui {
class SceneWindow;
}

class SceneWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SceneWindow(QWidget *parent = nullptr);
    ~SceneWindow();

    void setFileName(QString);
    QString fileName() const;

    void setSourceImage(const QImage &);
    const QImage & sourceImage() const;

    virtual QSize sizeHint() const;
    QLabel * canvas() const;

    void setImage(const Zx4Image &);
    const Zx4Image & image() const;

    void setAttr(QMap<int, QMap<int, int>>);
    QMap<int, QMap<int, int>> attr() const;
private slots:
    void drawGrid();
private:
    Ui::SceneWindow *ui;

    QString file_name;

    Zx4Image scene;
    QMap<int, QMap<int, int>> scene_attr; // 32x24

    QImage src_imp_img;

    bool eventFilter(QObject *, QEvent *);
};

#endif // SCENEWINDOW_H
