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

    void on_actionQuit_triggered();
    void on_palettes_itemSelectionChanged();
    void on_actionResize_canvas_triggered();
    void on_actionResize_image_triggered();
    void on_actionImport_triggered();
    void on_palettes_itemClicked(QListWidgetItem *item);
    void on_actionNew_triggered();
    void on_actionImportWizardApply_triggered();
    void on_actionInvert_colors_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_as_triggered();
    void on_wizardPaletteBox_currentIndexChanged(int index);
    void on_actionSave_triggered();
    void on_actionImport_scene_triggered();
    void on_actionExport_scene_triggered();
    void on_actionColor_00_toggled(bool);
    void on_actionColor_01_toggled(bool);
    void on_actionColor_10_toggled(bool);
    void on_actionColor_11_toggled(bool);

private:
    Ui::MainWindow *ui;

    enum ZxColorEnum {ZxBlack = 0, ZxBlue, ZxRed, ZxMagenta, ZxGreen, ZxCyan, ZxYellow, ZxWhite, ZxBrightBlack, ZxBrightBlue, ZxBrightRed, ZxBrightMagenta, ZxBrightGreen, ZxBrightCyan, ZxBrightYellow, ZxBrightWhite};
    QMap<ZxColorEnum, QColor> color_table;
    QMap<ZxColorEnum, QString> color_name;
    QMap<int, QList<ZxColorEnum>> color_palette_table;

    bool eventFilter(QObject *, QEvent *);

    int grid_x = 64, grid_y = 64;
    int current_color = 0;
    QList<QColor> colors() const;
    QList<QList<QColor>> palettes;
    QMap<QColor, QList<int>> palettes_by_color;
    QMap<QPair<QColor, QColor>, QList<int>> palettes_by_2colors;
    QMap<QPair<QColor, QColor>, QMap<QColor, QList<int>>> palettes_by_3colors;
    QMap<QPair<QColor, QColor>, QMap<QPair<QColor, QColor>, QList<int>>> palettes_by_4colors;

    struct Zx4Image
    {
        QVector<QVector<char>> color_layer0, color_layer1, alpha_layer;
        bool isNull() const { return color_layer0.isEmpty() || color_layer1.isEmpty() || alpha_layer.isEmpty(); }
        void resize(int w, int h)
        {
            color_layer0.resize(h);
            color_layer1.resize(h);
            alpha_layer.resize(h);
            for (int y = 0; y < h; ++y)
            {
                color_layer0[y].resize(w);
                color_layer1[y].resize(w);
                alpha_layer[y].resize(w);
            }
        }
    };
    Zx4Image image, stored_image;
    QPoint old_grid_pos;
    QString current_file_name;
    QImage src_imp_image;

    Zx4Image scene;
    QMap<int, QMap<int, int>> scene_attr; // 32x24

    int colorIndex(const Zx4Image & img, int x, int y) const;
    void drawPixels(QPainter * p, int step);
    void drawDot(int x, int y);
    void drawBrush(int x, int y);
    void eraseDot(int x, int y);
    void moveImage(int dx, int dy);
    void resizeGrid();
    void createPalettes();
    QImage scaleImage(QImage img, int w, int h) const;
    QMultiMap<int, QColor> colorDistribution(QImage img, bool use_alpha = true) const;
    bool alikeColors(QColor c1, QColor c2, int hue_precision = 20) const;
    QMap<QColor, int> colorDistribution4(QImage img, int scale_x = 256, int scale_y = 256) const;
    QMultiMap<double, int> selectPalette(QMap<QColor, int> color_distrib4) const;
    QMultiMap<double, int> selectPalette(QImage img, int scale_x = 256, int scale_y = 256) const;
    QImage importImage(QImage img, int palette_number, bool use_alpha);
    void drawImage(QImage img);

    void showEvent(QShowEvent *);
    void resizeEvent(QResizeEvent *);
};
#endif // MAINWINDOW_H
