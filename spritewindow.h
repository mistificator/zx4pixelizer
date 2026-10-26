#ifndef SPRITEWINDOW_H
#define SPRITEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QMap>

#include "zx4pixelizer.h"

namespace Ui {
class SpriteWindow;
}

class SpriteWindow : public QWidget, public Zx4BaseObject
{
    Q_OBJECT

public:
    explicit SpriteWindow(const Zx4Palette & palettes, QWidget *parent = nullptr);
    ~SpriteWindow();
    virtual void setFileName(QString);
    virtual void setSourceImage(const QList<QImage> &);
    virtual void setImage(const QList<const Zx4Image *> &);
    virtual QSize sizeHint() const;

    enum ActionEnum {ActionPencil = 0, ActionBrush, ActionFill, ActionMove, ActionEraser};
    void setAction(ActionEnum, QAction *);
    void setBrushSize(int s);
    int brushSize() const;
    void setEraserSize(int s);
    int eraserSize() const;

    void setColors(QList<QColor>);
    QList<QColor> colors() const;
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
    void eraseBrush(int x, int y);
    void floodFill(int x, int y);
    void moveImage(int dx, int dy);
    void drawImage(QImage img);
    void invertColors();

signals:
    void previewUpdateIsNeeded();
    void undoRedoMenuUpdateIsNeeded();

private slots:
    void drawGrid();
    void on_actionAnimate_toggled(bool);
    void on_actionStop_triggered();
    void on_frameBox_valueChanged(int);
    void animationFrame();

private:
    Ui::SpriteWindow *ui;
    Zx4Image stored_sprite;
    QPoint old_grid_pos;
    QMap<ActionEnum, QAction *> acts;
    QList<QColor> clrs;
    int p_idx = 0;
    int brush_size = 5;
    int eraser_size = 5;
    bool eventFilter(QObject *, QEvent *);
};

#endif // SPRITEWINDOW_H
