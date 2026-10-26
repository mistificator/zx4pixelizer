#ifndef SPRITEWINDOW_H
#define SPRITEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QMap>
#include <QVector>

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

    void updateCanvasCursor();
    void updateFrameStrip();
    void invalidateThumbnails(int frame_idx = -1);
    void selectFrame(int index);
    bool insertFrameAt(int insert_idx, const Zx4Image & frame_img, const QString & history_desc);
    void clearCurrentFrame(const QString & history_desc = "Clear frame");
    void cutCurrentFrame();

signals:
    void previewUpdateIsNeeded();
    void undoRedoMenuUpdateIsNeeded();

protected:
    void onHistoryRestored() override;

private slots:
    void drawGrid();
    void on_actionAnimate_toggled(bool);
    void on_actionStop_triggered();
    void on_actionAddFrame_triggered();
    void on_actionRemoveFrame_triggered();
    void animationFrame();

private:
    Ui::SpriteWindow *ui;
    QScrollArea * frame_scroll = nullptr;
    QWidget * frame_strip = nullptr;
    mutable QVector<QImage> thumb_cache;
    int drag_initial_frame = -1;
    int drag_source_frame = -1;
    bool is_dragging_frame = false;
    QPoint drag_start_pos;
    QPoint last_draw_pos = QPoint(-1, -1);

    Zx4Image stored_sprite;
    QPoint old_grid_pos;
    QMap<ActionEnum, QAction *> acts;
    QList<QColor> clrs;
    int p_idx = 0;
    int brush_size = 5;
    int eraser_size = 5;

    static SpriteWindow * spriteWindowAtGlobalPos(const QPoint & globalPos);
    void drawFrameStrip();
    int frameIndexAtPos(const QPoint & pos) const;
    QRect frameCellRect(int index) const;
    QImage renderFrameThumbnail(int frame_idx, const QSize & box_size) const;
    void ensureFrameVisible(int index);
    bool eventFilter(QObject *, QEvent *) override;
};

#endif // SPRITEWINDOW_H
