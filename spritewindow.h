#ifndef SPRITEWINDOW_H
#define SPRITEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QMap>
#include "zx4pixelizer.h"

namespace Ui {
class SpriteWindow;
}

class SpriteWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SpriteWindow(QWidget *parent = nullptr);
    ~SpriteWindow();

    void setFileName(QString);
    QString fileName() const;

    virtual QSize sizeHint() const;
    QLabel * canvas() const;
    const Zx4Image & image() const;

    void setSourceImage(QImage);
    const QImage & sourceImage() const;

    enum ActionEnum {ActionPencil = 0, ActionBrush, ActionMove, ActionEraser};
    void setAction(ActionEnum, QAction *);

    void setColors(QList<QColor>);
    QList<QColor> colors() const;

    void setCurrentColorIndex(int);
    int currentColorIndex() const;

    void setPaletteIndex(int);
    int paletteIndex() const;

    void clearCanvas(QSize);
    void setCanvasSize(QSize);
    QSize canvasSize() const;

    void resizeImage(QSize);

    int colorIndex(const Zx4Image & img, int x, int y) const;
    void drawPixels(QPainter * p, int step);
    void drawDot(int x, int y);
    void drawBrush(int x, int y);
    void eraseDot(int x, int y);
    void moveImage(int dx, int dy);
    void drawImage(QImage img);

    void invertColors();
signals:
    void previewUpdateIsNeeded();
private slots:
    void drawGrid();

private:
    Ui::SpriteWindow *ui;

    bool eventFilter(QObject *, QEvent *);

    int grid_x = 64, grid_y = 64;
    Zx4Image img, stored_img;
    QPoint old_grid_pos;
    QString current_file_name;
    QImage src_imp_img;

    QMap<ActionEnum, QAction *> acts;
    QList<QColor> clrs;
    int current_color_index = 0;
    int p_idx = 0;
};

#endif // SPRITEWINDOW_H
