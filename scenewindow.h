#ifndef SCENEWINDOW_H
#define SCENEWINDOW_H

#include <QWidget>
#include <QMap>
#include <QLabel>
#include "zx4pixelizer.h"

namespace Ui {
class SceneWindow;
}

class SceneWindow : public QWidget, public Zx4BaseObject
{
    Q_OBJECT

public:
    explicit SceneWindow(const Zx4Palette & palettes, QWidget *parent = nullptr);
    ~SceneWindow();

    virtual void setFileName(QString);
    const QImage & renderedImage() const;

    virtual void setSourceImage(const QList<QImage> &);

    virtual void setImage(const QList<const Zx4Image *> &);
    virtual QSize sizeHint() const;

    void setAttr(Zx4ImageWithAttributes::Attr);
    Zx4ImageWithAttributes::Attr attr() const;

    QPoint currentChar() const;
    void setAttr(QPoint, int p_idx);
    void setAttr(int p_idx);

    int selectPalette(QImage small_quad, Zx4Palette::SelectPaletteMethod method = Zx4Palette::DefaultSelectPaletteMethod) const;
    void importChar(QPoint, int p_idx, QImage small_quad);

    void setPixel(QPoint, int color_index);
    void invertColors();
signals:
    void previewUpdateIsNeeded();
    void charSelected(int p_idx);
private slots:
    void drawGrid();
private:
    Ui::SceneWindow *ui;
    Zx4ImageWithAttributes & scene;

    bool eventFilter(QObject *, QEvent *);

    QImage rdr_img;
    QPoint selected_char, update_char;
    bool checkPalette(QImage small_quad, int p_idx) const;
};

#endif // SCENEWINDOW_H
