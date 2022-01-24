#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_baseColor_activated(int index);
    void on_actionQuit_triggered();
    void on_palettes_itemSelectionChanged();
    void on_color00_clicked();
    void on_color01_clicked();
    void on_color10_clicked();
    void on_color11_clicked();

    void drawGrid();
    void updateColorArea();
    void on_actionResize_canvas_triggered();

    void on_actionResize_image_triggered();

private:
    Ui::MainWindow *ui;

    enum ZxColorEnum {ZxBlack = 0, ZxBlue, ZxRed, ZxMagenta, ZxGreen, ZxCyan, ZxYellow, ZxWhite, ZxBrightBlack, ZxBrightBlue, ZxBrightRed, ZxBrightMagenta, ZxBrightGreen, ZxBrightCyan, ZxBrightYellow, ZxBrightWhite};
    QMap<ZxColorEnum, QColor> color_table;
    QMap<ZxColorEnum, QString> color_name;

    bool eventFilter(QObject *, QEvent *);

    int grid_x = 32, grid_y = 32;
    int current_color = 0;
    QList<QColor> colors() const;

    struct Zx4Image
    {
        QVector<QVector<char>> color_layer0, color_layer1, alpha_layer;
    };
    Zx4Image image, stored_image;
    QPoint old_grid_pos;

    void drawPixels(QPainter * p, int step);
    void drawDot(int x, int y);
    void eraseDot(int x, int y);
    void moveImage(int dx, int dy);
    void resizeGrid();
};
#endif // MAINWINDOW_H
