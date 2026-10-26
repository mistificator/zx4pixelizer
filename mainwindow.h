#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QToolButton>
#include <QActionGroup>
#include <QMdiSubWindow>
#include <QProgressDialog>
#include <QSharedPointer>

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
    static QProgressDialog * createProgress(QWidget * parent, QString text);

private slots:
    void selectPalette(int p_idx);
    void undoRedoMenuUpdate();
    void on_actionQuit_triggered();
    void on_actionResize_canvas_triggered();
    void on_actionResize_image_triggered();
    void on_actionAbout_ZX4_Pixelizer_triggered();
    void on_palettes_itemSelectionChanged();
    void on_palettes_itemClicked(QListWidgetItem *item);
    void on_wizardPaletteBox_currentIndexChanged(int index);
    void on_actionInvert_colors_triggered();
    void on_actionColor_00_toggled(bool);
    void on_actionColor_01_toggled(bool);
    void on_actionColor_10_toggled(bool);
    void on_actionColor_11_toggled(bool);
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_as_triggered();
    void on_actionImport_triggered();
    void on_actionImportWizardApply_triggered();
    void on_actionSave_triggered();
    void on_actionImport_scene_triggered();
    void on_actionExport_scene_triggered();
    void on_actionExport_triggered();
    void on_mdiArea_subWindowActivated(QMdiSubWindow *msw);
    void on_paletteIndex_currentIndexChanged(int index);
    void on_presetBox_currentIndexChanged(int);
    void on_addPreset_clicked();
    void on_removePreset_clicked();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionClear_triggered();

protected:
    bool eventFilter(QObject *, QEvent *) override;
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void closeEvent(QCloseEvent *) override;

private:
    Ui::MainWindow *ui;
    SpriteWindow * current_sprite = nullptr;
    SceneWindow * current_scene = nullptr;
    QSharedPointer<Zx4Image> clipboard_frame;
    Zx4Palette palettes;
    double preview_split_pos = 0.5;
    int current_brush_size = 5;
    int current_eraser_size = 5;

    QComboBox * toolbarPresetBox = nullptr;
    QCheckBox * toolbarDitherBox = nullptr;
    QToolButton * toolbarReimportBtn = nullptr;
    QActionGroup * presetMenuGroup = nullptr;

    void createPalettes();
    void syncPresetsUI(const QString & selectedPreset = QString());
    void syncAlphaState(bool checked);
    void syncDitherState(bool checked);
    void updateToolbarWidgetsHeight();
    bool askResizeSize(const QString & title, QSize & ioSize);
    static QString configFilePath();
    void saveSettings() const;
    void loadSettings();

    QImage importImage(QImage img, int palette_number, bool use_alpha);
    Zx4Palette::SelectPaletteMethod selectPaletteMethod() const;
    QImage applyTransforms(const Zx4BaseObject & obj, const QImage & src, QColor bg_color = QColor(0, 0, 0, 0)) const;
    void importScene(QImage img, QString filename = QString());
    void openSprite(QList<QImage> img, QString filename = QString());
    void importSprite(QList<QImage> img, QString filename = QString(), QSize sz = QSize(), int p_idx = -1);
    void importSpriteZx4Format(QString filename);
    void exportSpriteZx4Format(QString filename);
    void exportScreens(const Zx4ImageWithAttributes & image, QString filename_scr0, QString filename_scr1 = QString());
    void exportTape(const Zx4ImageWithAttributes & image, QString filename);
    void importParsFromWidget(Zx4ImportPars &);
    void importParsToWidget(const Zx4ImportPars &);
    void importParsFromWidgetToObject(Zx4BaseObject &);
    void importParsFromObjectToWidget(const Zx4BaseObject &);
    void reimport(SpriteWindow *, int p_idx = -1);
    void reimport(SceneWindow *);
    void updateUndoRedoMenu(Zx4BaseObject * zbo);
};

#endif // MAINWINDOW_H
