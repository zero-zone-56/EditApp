#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QFileSystemModel;
class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    QFileSystemModel *m_FileSystemModel;

    QTabWidget *tabWidget;

    QPixmap m_PixMap;

private:
    QString getFile(bool save=false);

    void loadFileToTab(const QString &filePath);

    int findTabByFileName(const QString &fileName);

    void updateTabContent(int tabIndex, const QString &content);

    void createNewTab(const QString &title, const QString &content, const QString &fileName);

    void markTabAsModified(const QString &originalTitle);

    void refreshFileTree(const QString& dirPath);

    void createNewFileInDirectory(const QString &directoryPath);

    void createNewFolderInDirectory(const QString &directoryPath);

    void openImageFile(const QString &filePath);

    void createImageTab(const QString &filePath);

    // 添加缩放功能
    void setupImageLabelContextMenu(QLabel *imageLabel);
    //
    void showImageContextMenu(QLabel *imageLabel, const QPoint &pos);

    void fitImageToWindow(QLabel *imageLabel);

    void showImageOriginalSize(QLabel *imageLabel);

    void zoomImage(QLabel *imageLabel, qreal factor);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_act_New_triggered();

    void on_tabWidget_tabCloseRequested(int index);

    void on_act_Open_triggered();

    void on_act_Save_triggered();

    void on_treeView_clicked(const QModelIndex &index);

    void on_act_SetRoot_triggered();

    void on_act_Close_triggered();

    void on_act_CloseAll_triggered();

    void on_treeView_customContextMenuRequested(const QPoint &pos);

    void on_act_NewDir_triggered();

    void on_act_Cut_triggered();

    void on_act_Copy_triggered();

    void on_act_Paste_triggered();

    void on_act_Undo_triggered();

    void on_act_Redo_triggered();

    void on_act_FontBold_triggered(bool checked);

    void on_act_FontItalic_triggered(bool checked);

    void on_act_FontUnderline_triggered(bool checked);

    void on_act_FontSize_triggered();

    void on_act_FontStyle_triggered();

    void on_act_DockVisiable_triggered(bool checked);

    void on_act_DockFloat_triggered(bool checked);

    void on_dockWidget_visibilityChanged(bool visible);

    void on_dockWidget_topLevelChanged(bool topLevel);

    void on_act_LinkDB_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
