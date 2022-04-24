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
    explicit SceneWindow(const Zx4Palette & palettes, QWidget *parent = nullptr);
    ~SceneWindow();

    void setFileName(QString);
    QString fileName() const;

    void setSourceImage(const QImage &);
    const QImage & sourceImage() const;
    QImage & transformedImage();
    const QImage & renderedImage() const;

    virtual QSize sizeHint() const;
    QLabel * canvas() const;

    void setImage(const Zx4ImageWithAttributes &);
    const Zx4ImageWithAttributes & image() const;

    void setAttr(Zx4ImageWithAttributes::Attr);
    Zx4ImageWithAttributes::Attr attr() const;

    QPoint currentChar() const;
    void setAttr(QPoint, int p_idx);
    void setAttr(int p_idx);

    int selectPalette(QImage small_quad, Zx4Palette::SelectPaletteMethod method = Zx4Palette::DefaultSelectPaletteMethod) const;
    void importChar(QPoint, int p_idx, QImage small_quad);

    void setCurrentColorIndex(int);
    int currentColorIndex() const;
    void setPixel(QPoint, int color_index);

    void invertColors();
signals:
    void previewUpdateIsNeeded();
    void charSelected(int p_idx);
private slots:
    void drawGrid();
private:
    Ui::SceneWindow *ui;
    const Zx4Palette & palettes;

    bool eventFilter(QObject *, QEvent *);

    QString file_name;

    Zx4ImageWithAttributes scene;

    QImage src_imp_img, transformed_img, rdr_img;
    int grid_x = 1, grid_y = 1;
    QPoint selected_char, update_char;
    int current_color_index = 0;
    bool checkPalette(QImage small_quad, int p_idx) const;
};

#endif // SCENEWINDOW_H
