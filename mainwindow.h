#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QListWidgetItem;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void drawGrid();
    void updateColorArea();

    void on_actionQuit_triggered();
    void on_palettes_itemSelectionChanged();
    void on_color00_clicked();
    void on_color01_clicked();
    void on_color10_clicked();
    void on_color11_clicked();
    void on_actionResize_canvas_triggered();
    void on_actionResize_image_triggered();
    void on_actionImport_triggered();
    void on_palettes_itemClicked(QListWidgetItem *item);
    void on_actionNew_triggered();

private:
    Ui::MainWindow *ui;

    enum ZxColorEnum {ZxBlack = 0, ZxBlue, ZxRed, ZxMagenta, ZxGreen, ZxCyan, ZxYellow, ZxWhite, ZxBrightBlack, ZxBrightBlue, ZxBrightRed, ZxBrightMagenta, ZxBrightGreen, ZxBrightCyan, ZxBrightYellow, ZxBrightWhite};
    QMap<ZxColorEnum, QColor> color_table;
    QMap<ZxColorEnum, QString> color_name;

    bool eventFilter(QObject *, QEvent *);

    int grid_x = 64, grid_y = 64;
    int current_color = 0;
    QList<QColor> colors() const;

    struct Zx4Image
    {
        QVector<QVector<char>> color_layer0, color_layer1, alpha_layer;
        bool isNull() const { return color_layer0.isEmpty() || color_layer1.isEmpty() || alpha_layer.isEmpty(); }
    };
    Zx4Image image, stored_image;
    QPoint old_grid_pos;
    QImage src_imp_image;

    void drawPixels(QPainter * p, int step);
    void drawDot(int x, int y);
    void drawBrush(int x, int y);
    void eraseDot(int x, int y);
    void moveImage(int dx, int dy);
    void resizeGrid();
    void createPalettes();

    void showEvent(QShowEvent *);
    void resizeEvent(QResizeEvent *);
};
#endif // MAINWINDOW_H
