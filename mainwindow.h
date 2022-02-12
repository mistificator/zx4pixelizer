#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QLabel>
#include <QMdiSubWindow>
#include "zx4pixelizer.h"
#include "spritewindow.h"
#include "scenewindow.h"

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
    void selectPalette(int p_idx);

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
    void on_actionExport_triggered();
    void on_actionAbout_ZX4_Pixelizer_triggered();
    void on_mdiArea_subWindowActivated(QMdiSubWindow *msw);
private:
    Ui::MainWindow *ui;
    SpriteWindow * current_sprite = 0;
    SceneWindow * current_scene = 0;
    Zx4Palette palettes;

    bool eventFilter(QObject *, QEvent *);

    void createPalettes();
    QImage scaleImage(QImage img, int w, int h) const;
    QMultiMap<int, QColor> colorDistribution(QImage img, bool use_alpha = true) const;
    bool alikeColors(QColor c1, QColor c2, int hue_precision = 20, int light_precision = 32, int satur_precision = 32) const;
    QMap<QColor, int> colorDistribution4(QImage img) const;
    QMultiMap<double, int> selectPalette(QMap<QColor, int> color_distrib4) const;
    QMultiMap<double, int> selectPalette(QImage img) const;
    QImage importImage(QImage img, int palette_number, bool use_alpha);

    void showEvent(QShowEvent *);
    void resizeEvent(QResizeEvent *);
};
#endif // MAINWINDOW_H
