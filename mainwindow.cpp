#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileSystemModel>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QMessageBox>

QString MainWindow::getFile(bool save)
{
    QString curPath=QCoreApplication::applicationDirPath(); // 返回应用程序可执行文件所在的路径
    QString dlgTitle="选择一个文件";
    QString filter="程序文件(*.h *.cpp);;文本文件(*.txt);;所有文件(*.*)";

    QString aFileName;
    if(save){
        aFileName=QFileDialog::getSaveFileName();   // 选择保存一个文件，返回保存文件的文件名
        // 这里保存文件时默认有对话框,改进地方应该是如果保存现有文件时,可以直接写入内容,不会弹出文件对话框;如果保存新文件,应该弹出对话框可以显示保存的文件夹

    }
    else
        aFileName=QFileDialog::getOpenFileName(this,dlgTitle,curPath,filter);  //选择打开一个文件，返回选择文件的文件名

    if(aFileName.isEmpty()) return aFileName;

    QFileInfo fileInfo(aFileName);  // QFileInfo 类用于获取文件的各种信息
    QDir::setCurrent(fileInfo.absoluteFilePath());
    return aFileName;
}

void MainWindow::loadFileToTab(const QString &filePath)
{
    QFile file(filePath);  // QFile 除了能用于进行文件内容的读写，还有一些静态函数和接口函数可用于文件操作，例如复制文件、删除文件、重命名文件等
    // QFile 的父类是 QFileDevice，QFileDevice 提供了文件交互操作的底层功能。QFileDevice 的父类是 QIODevice
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件:" << filePath;
        return;
    }

    // 读取文件内容
    QTextStream in(&file);  // QTextStream 是能与 I/O 设备类结合来为读写文本数据提供一些简便接口函数的类。QTextStream可以和 QIODevice 的各种子类结合使用，如 QFile、QSaveFile、QTcpSocket、QUdpSocket 等 I/O 设备类。
    QString content = in.readAll();
    file.close();

    // 获取文件名（不带路径）
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    // qDebug()<< fileName;   // "新建文本文档2.txt"

    // 检查是否已经打开该文件
    int existingTabIndex = findTabByFileName(fileName);
    // qDebug() << existingTabIndex;  // -1

    if (existingTabIndex >= 0) {
        // 如果已经打开，切换到该标签页
        ui->tabWidget->setCurrentIndex(existingTabIndex);
        updateTabContent(existingTabIndex, content);
    } else {
        // 创建新的标签页
        createNewTab(fileName, content, fileName);
    }
}

int MainWindow::findTabByFileName(const QString &fileName)
{
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        QString tabText = ui->tabWidget->tabText(i);
        // 注意：tabText可能包含*标记（表示已修改）
        if (tabText.replace("*", "") == fileName) {
            // qDebug()<<ui->tabWidget->currentIndex();  // 打印出当前标签页的索引
            return i;
        }
    }
    return -1;
}

void MainWindow::updateTabContent(int tabIndex, const QString &content)
{
    QWidget* tabWidget = ui->tabWidget->widget(tabIndex);
    if (!tabWidget) return;

    QPlainTextEdit* textEdit = qobject_cast<QPlainTextEdit*>(tabWidget);
    if (!textEdit) {
        // 如果不是QPlainTextEdit，尝试在子组件中查找
        textEdit = tabWidget->findChild<QPlainTextEdit*>();
    }

    if (textEdit) {
        textEdit->setPlainText(content);   // 将文本编辑器的文本更改为字符串text 。之前的文本将被删除。
    }

    // 连接QPlainTextEdit组件的一些常规操作,包括cut, copy, paste
    connect(textEdit,&QPlainTextEdit::copyAvailable,this,[this,textEdit](bool enabled){
        ui->act_Cut->setEnabled(enabled);
        ui->act_Copy->setEnabled(enabled);
        ui->act_Paste->setEnabled(textEdit->canPaste());
    });

    // redo和undo的功能写信号与槽的连接
    connect(textEdit,&QPlainTextEdit::redoAvailable,this,[this](bool enabled){
        ui->act_Redo->setEnabled(enabled);
    });
    connect(textEdit,&QPlainTextEdit::undoAvailable,this,[this](bool enabled){
        ui->act_Undo->setEnabled(enabled);
    });

    // 粗体、斜体和下划线写信号与槽的连接
    QTextCharFormat fmt=textEdit->currentCharFormat();
    connect(textEdit,&QPlainTextEdit::selectionChanged,this,[this,fmt](){
        ui->act_FontBold->setChecked(fmt.font().bold());
        ui->act_FontItalic->setChecked(fmt.font().italic());
        ui->act_FontUnderline->setChecked(fmt.font().underline());
    });
}

void MainWindow::createNewTab(const QString &title, const QString &content, const QString &fileName)
{
    // 创建QPlainTextEdit
    QPlainTextEdit* textEdit = new QPlainTextEdit();
    textEdit->setPlainText(content);
    int cur=ui->tabWidget->addTab(textEdit,
                                    QString::asprintf("Table %d",ui->tabWidget->count()));
    ui->tabWidget->setCurrentIndex(cur);
    ui->tabWidget->setTabText(cur,fileName);

    // 可选：设置文本编辑器属性
    textEdit->setLineWrapMode(QPlainTextEdit::NoWrap); // 不自动换行
    QFont font("Consolas", 10);  // 使用等宽字体
    textEdit->setFont(font);

    // 连接文本修改信号（用于标记未保存）
    connect(textEdit, &QPlainTextEdit::textChanged, this, [this, title]() {
        markTabAsModified(title);
    });

    // 添加到TabWidget
    int newTabIndex = ui->tabWidget->addTab(textEdit, title);
    ui->tabWidget->setCurrentIndex(newTabIndex);

    // 连接QPlainTextEdit组件的一些常规操作,包括cut, copy, paste
    connect(textEdit,&QPlainTextEdit::copyAvailable,this,[this,textEdit](bool enabled){
        ui->act_Cut->setEnabled(enabled);
        ui->act_Copy->setEnabled(enabled);
        ui->act_Paste->setEnabled(textEdit->canPaste());
        ui->act_Undo->setEnabled(enabled);
        ui->act_Redo->setEnabled(enabled);
    });

    // redo和undo的功能写信号与槽的连接
    connect(textEdit,&QPlainTextEdit::redoAvailable,this,[this](bool enabled){
        ui->act_Redo->setEnabled(enabled);
    });
    connect(textEdit,&QPlainTextEdit::undoAvailable,this,[this](bool enabled){
        ui->act_Undo->setEnabled(enabled);
    });

    // 粗体、斜体和下划线写信号与槽的连接
    QTextCharFormat fmt=textEdit->currentCharFormat();
    connect(textEdit,&QPlainTextEdit::selectionChanged,this,[this,fmt](){
        ui->act_FontBold->setChecked(fmt.font().bold());
        ui->act_FontItalic->setChecked(fmt.font().italic());
        ui->act_FontUnderline->setChecked(fmt.font().underline());
    });
}

void MainWindow::markTabAsModified(const QString &originalTitle)
{
    int currentIndex = ui->tabWidget->currentIndex();
    QString currentTitle = ui->tabWidget->tabText(currentIndex);

    // 如果标题还没有*标记，则添加
    if (!currentTitle.endsWith("*")) {
        ui->tabWidget->setTabText(currentIndex, currentTitle + "*");
    }
}
#include <QTimer>
void MainWindow::refreshFileTree(const QString &dirPath)
{

    QFileSystemModel* model = qobject_cast<QFileSystemModel*>(ui->treeView->model());
    if (model) {
        QModelIndex dirIndex = model->index(dirPath);
        if (dirIndex.isValid()) {
            // 延迟刷新，避免频繁操作
            QTimer::singleShot(100, this, [model, dirIndex]() {
                model->fetchMore(dirIndex);
            });
        }
    }
}

#include <QInputDialog>
void MainWindow::createNewFileInDirectory(const QString &directoryPath)
{

    // 弹出对话框获取文件名
    bool ok;
    QString fileName = QInputDialog::getText(this, "新建文件",
                                             "请输入文件名:",
                                             QLineEdit::Normal,
                                             "新建文件.txt", &ok);

    if (!ok || fileName.isEmpty()) return;

    // 确保有扩展名
    if (!fileName.contains('.')) {
        fileName += ".txt";
    }

    QString fullPath = QDir(directoryPath).filePath(fileName);  // 如果 fileName 是不带有路径的文件名，函数返回值是带有操作目录的完整文件名

    // 检查文件是否已存在
    if (QFile::exists(fullPath)) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "文件已存在",
                                      "文件已存在，是否覆盖？",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) return;
    }

    // 创建空文件
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.close();

        // 在TabWidget中打开新文件
        loadFileToTab(fullPath);

        // 刷新文件树
        refreshFileTree(directoryPath);

        qDebug() << "文件创建并打开:" << fullPath;
    } else {
        QMessageBox::warning(this, "错误", "无法创建文件");
    }

}

void MainWindow::createNewFolderInDirectory(const QString &directoryPath)
{

    bool ok;
    QString folderName = QInputDialog::getText(this, "新建文件夹",
                                               "请输入文件夹名:",
                                               QLineEdit::Normal,
                                               "新建文件夹", &ok);

    if (!ok || folderName.isEmpty()) return;

    QString fullPath = QDir(directoryPath).filePath(folderName);

    QDir dir(directoryPath);
    if (dir.mkdir(folderName)) {
        refreshFileTree(directoryPath);
        qDebug() << "文件夹创建成功:" << fullPath;
    } else {
        QMessageBox::warning(this, "错误", "无法创建文件夹");
    }

}

void MainWindow::openImageFile(const QString &filePath)
{
    // 获取文件名（不带路径）
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    // 检查是否已经打开该图片
    int existingTabIndex = findTabByFileName(fileName);
    if (existingTabIndex >= 0) {
        ui->tabWidget->setCurrentIndex(existingTabIndex);
        return;
    }

    // 加载图片
    if (!m_PixMap.load(filePath)) {
        QMessageBox::warning(this, "错误", "无法加载图片: " + filePath);
        return;
    }

    // 创建显示图片的标签页
    createImageTab(filePath);
}
#include <QScrollArea>
#include <QLabel>

void MainWindow::createImageTab(const QString &filePath)
{
    // 1. 创建Scroll Area作为容器
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setWidgetResizable(true);  // 允许缩放

    // 2. 创建QLabel来显示图片
    QLabel *imageLabel = new QLabel();
    imageLabel->setPixmap(m_PixMap);
    imageLabel->setScaledContents(false);  // 保持原始比例
    imageLabel->setAlignment(Qt::AlignCenter);

    // 3. 将QLabel放入Scroll Area
    scrollArea->setWidget(imageLabel);

    // 5. 添加到TabWidget
    QFileInfo fileInfo(filePath);
    QString title = fileInfo.fileName();
    int tabIndex = ui->tabWidget->addTab(scrollArea, title);
    ui->tabWidget->setCurrentIndex(tabIndex);

    // 6. 保存标签页类型信息
    scrollArea->setProperty("tabType", "image");
    scrollArea->setProperty("filePath", filePath);

    // 7. 可选：右键菜单支持图片操作
    setupImageLabelContextMenu(imageLabel);

    // 8. 可选：将工具栏上的行为挂靠在对应的函数上(自己写的)
    connect(ui->act_ZoomIn, &QAction::triggered, [this,imageLabel](){
        zoomImage(imageLabel,1.2);
    });    // 放大
    connect(ui->act_ZoomOut, &QAction::triggered, [this,imageLabel](){
        zoomImage(imageLabel,0.8);
    });   // 缩放
    connect(ui->act_ZoomRealSize, &QAction::triggered, [this,imageLabel]{
        showImageOriginalSize(imageLabel);
    });   // 实际大小
    connect(ui->act_ZoomFitWindow, &QAction::triggered, [this,imageLabel]{
        fitImageToWindow(imageLabel);
    });
}

void MainWindow::setupImageLabelContextMenu(QLabel *imageLabel)
{
    imageLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(imageLabel, &QLabel::customContextMenuRequested,
            [this, imageLabel](const QPoint &pos) {
                showImageContextMenu(imageLabel, pos);
            });
}
// 不知道这个pos指针指向哪里
void MainWindow::showImageContextMenu(QLabel *imageLabel, const QPoint &pos)
{
    QMenu menu;

    // 缩放菜单
    QMenu *zoomMenu = menu.addMenu("缩放");
    zoomMenu->addAction("适合窗口", [this, imageLabel]() {
        fitImageToWindow(imageLabel);
    });

    zoomMenu->addAction("实际大小", [this, imageLabel]() {
        showImageOriginalSize(imageLabel);
    });

    zoomMenu->addAction("放大", [this, imageLabel]() {
        zoomImage(imageLabel, 1.2);
    });

    zoomMenu->addAction("缩小", [this, imageLabel]() {
        zoomImage(imageLabel, 0.8);
    });

    // 显示菜单
    menu.exec(imageLabel->mapToGlobal(pos));
}

void MainWindow::fitImageToWindow(QLabel *imageLabel)
{
    QScrollArea *scrollArea = qobject_cast<QScrollArea*>(imageLabel->parentWidget()->parentWidget());
    if (!scrollArea) return;

    // QVariant var = imageLabel->property("originalPixmap");
    // QPixmap originalPixmap = var.value<QPixmap>();

    // if (originalPixmap.isNull()) return;  // 到这里函数就返回了
    // // 计算适合窗口的大小
    // QSize scrollSize = scrollArea->viewport()->size();
    // QPixmap scaledPixmap = originalPixmap.scaled(scrollSize,
    //                                              Qt::KeepAspectRatio,
    //                                              Qt::SmoothTransformation);
    // imageLabel->setPixmap(scaledPixmap);
    // imageLabel->setScaledContents(false);

    // 自己修改的
    int w=scrollArea->width();
    int realw=m_PixMap.width();
    QPixmap pix=m_PixMap.scaledToWidth(w-30);
    imageLabel->setPixmap(pix);

    int h=scrollArea->height();
    int realh=m_PixMap.height();
    pix=m_PixMap.scaledToHeight(h-30);
    imageLabel->setPixmap(pix);
}

void MainWindow::showImageOriginalSize(QLabel *imageLabel)
{
    // 自己写的
    imageLabel->setPixmap(m_PixMap);
}

void MainWindow::zoomImage(QLabel *imageLabel, qreal factor)
{
    QPixmap currentPixmap = imageLabel->pixmap(Qt::ReturnByValue);
    if (currentPixmap.isNull()) return;

    QSize newSize = currentPixmap.size() * factor;
    QPixmap scaledPixmap = currentPixmap.scaled(newSize,
                                                Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation);

    imageLabel->setPixmap(scaledPixmap);
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->act_DockVisiable->setChecked(Qt::Checked);

    ui->tabWidget->setVisible(true);
    ui->tabWidget->clear();
    ui->tabWidget->setTabsClosable(true);

    m_FileSystemModel=new QFileSystemModel(this);
    ui->treeView->setModel(m_FileSystemModel);
    m_FileSystemModel->setRootPath("");
    ui->treeView->setRootIndex(m_FileSystemModel->index(QDir::currentPath()));
    ui->treeView->setEditTriggers(QAbstractItemView::DoubleClicked);

    // 设置TreeView只显示第一列，其他三列隐藏
    ui->treeView->setColumnHidden(1,true);
    ui->treeView->setColumnHidden(2,true);
    ui->treeView->setColumnHidden(3,true);

    // 在TreeView中点击鼠标右键然后组件创建快捷菜单,就必须添加下面语句
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::on_treeView_customContextMenuRequested);
    //QPlainTextEdit 的 contextMenuPolicy 属性默认设置为这个值，在无须任何编程的情况下，运行时点击鼠标右键就会出现一个标准的编辑操作快捷菜单，只是菜单文字是英文的。

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_act_New_triggered()
{
    QPlainTextEdit *textEdit=new QPlainTextEdit(this);
    ui->tabWidget->addTab(textEdit,QString::asprintf("Doc %d",ui->tabWidget->count()));

    // QModelIndex index=m_FileSystemModel->index(QDir::currentPath());
    // m_FileSystemModel.

    // 连接QPlainTextEdit组件的一些常规操作,包括cut, copy, paste
    connect(textEdit,&QPlainTextEdit::copyAvailable,this,[this,textEdit](bool enabled){
        ui->act_Cut->setEnabled(enabled);
        ui->act_Copy->setEnabled(enabled);
        ui->act_Paste->setEnabled(textEdit->canPaste());
        ui->act_Undo->setEnabled(enabled);
        ui->act_Redo->setEnabled(enabled);
    });

    // redo和undo的功能写信号与槽的连接
    connect(textEdit,&QPlainTextEdit::redoAvailable,this,[this](bool enabled){
        ui->act_Redo->setEnabled(enabled);
    });
    connect(textEdit,&QPlainTextEdit::undoAvailable,this,[this](bool enabled){
        ui->act_Undo->setEnabled(enabled);
    });

    // 粗体、斜体和下划线写信号与槽的连接
    QTextCharFormat fmt=textEdit->currentCharFormat();
    connect(textEdit,&QPlainTextEdit::selectionChanged,this,[this,fmt](){
        ui->act_FontBold->setChecked(fmt.font().bold());
        ui->act_FontItalic->setChecked(fmt.font().italic());
        ui->act_FontUnderline->setChecked(fmt.font().underline());
    });
}
// 关闭page
void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    ui->tabWidget->removeTab(index);
}

#include <QTextEdit>
void MainWindow::on_act_Open_triggered()
{
    QString curPath=QCoreApplication::applicationDirPath();
    QString dlgTitle="选择一个文件";
    QString filter="程序文件(*.h *.cpp);;文本文件(*.txt);;图片(*.jpg);;所有文件(*.*)";
    bool save=false;
    QString aFileName;
    if(save){
        QFileDialog::getSaveFileName();
        // aFileName=QFileDialog::getSaveFileName();   // 上面的语句应该修改为这个，但应该是我理解错了
    }
    else
        aFileName=QFileDialog::getOpenFileName(this,dlgTitle,curPath,filter);

    if(aFileName.isEmpty()) return;

    QFileInfo fileInfo(aFileName);

    // 判断是不是显示图片
    if (fileInfo.isFile()) {
        QString suffix = fileInfo.suffix().toLower();
        QString filePath = fileInfo.absoluteFilePath();

        // 图片文件扩展名
        QStringList imageFileExtensions = {"jpg", "jpeg", "png", "bmp",
                                           "gif", "ico", "webp", "tiff"};

        if (imageFileExtensions.contains(suffix)) {
            // 打开图片文件
            openImageFile(filePath);
            return;
        }
    }

    QDir::setCurrent(fileInfo.absoluteFilePath());
    QFile aFile(aFileName);

    if(!aFile.exists())
        return;

    if(!aFile.open(QIODevice::ReadOnly|QIODevice::Text))
        return;

    QPlainTextEdit *textEdit=new QPlainTextEdit(this);  // 这里我以为是这个QPlainTextEdit组件加入到TabWidget容器中，所以父组件是之前写的是ui->tabWidget，结果是QPlaiTextEdit是本身作为容器，可以直接用this指针
    int cur=ui->tabWidget->addTab(textEdit,
                                    QString::asprintf("Table %d",ui->tabWidget->count()));
    textEdit->appendPlainText(aFile.readAll());
    ui->tabWidget->setCurrentIndex(cur);
    ui->tabWidget->setTabText(cur,fileInfo.fileName());

    aFile.close();

    // 连接QPlainTextEdit组件的一些常规操作,包括cut, copy, paste
    connect(textEdit,&QPlainTextEdit::copyAvailable,this,[this,textEdit](bool enabled){
        ui->act_Cut->setEnabled(enabled);
        ui->act_Copy->setEnabled(enabled);
        ui->act_Paste->setEnabled(textEdit->canPaste());
        ui->act_Undo->setEnabled(enabled);
        ui->act_Redo->setEnabled(enabled);
    });

    // redo和undo的功能写信号与槽的连接
    connect(textEdit,&QPlainTextEdit::redoAvailable,this,[this](bool enabled){
        ui->act_Redo->setEnabled(enabled);
    });
    connect(textEdit,&QPlainTextEdit::undoAvailable,this,[this](bool enabled){
        ui->act_Undo->setEnabled(enabled);
    });

    // 粗体、斜体和下划线写信号与槽的连接
    QTextCharFormat fmt=textEdit->currentCharFormat();
    connect(textEdit,&QPlainTextEdit::selectionChanged,this,[this,fmt](){
        ui->act_FontBold->setChecked(fmt.font().bold());
        ui->act_FontItalic->setChecked(fmt.font().italic());
        ui->act_FontUnderline->setChecked(fmt.font().underline());
    });
}

// 保存函数
void MainWindow::on_act_Save_triggered()
{
    QFile aFile(getFile(true));
    if(!aFile.open(QIODevice::WriteOnly|QIODevice::Text))
        return;

    int cur = ui->tabWidget->currentIndex();
    QWidget* currentWidget = ui->tabWidget->widget(cur);

    // 这里遇到第一个难题

    //ui->tabWidget->widget(cur) 返回的本身就是 QPlainTextEdit，而不是包含 QPlainTextEdit 的容器！
    //这说明你在添加Tab页面时，直接把QPlainTextEdit作为Tab页面，而不是把QPlainTextEdit放在一个容器Widget中。

    // 先尝试作为QPlainTextEdit获取
    QPlainTextEdit* textEdit = qobject_cast<QPlainTextEdit*>(currentWidget);
    if(!textEdit) {
        // 如果不是QPlainTextEdit，再尝试查找子组件
        textEdit = currentWidget->findChild<QPlainTextEdit*>();
    }

    if(textEdit) {
        QString text = textEdit->toPlainText();
        // qDebug() << text;
        aFile.write(text.toUtf8(), text.toUtf8().length());
    } else {
        qDebug() << "未找到文本编辑框";
    }

    aFile.close();

    // 保存文件时候把文件名称上的*去掉
    {
        // currentIndex 跟 本函数最上面的 cur 是相同的
        int currentIndex = ui->tabWidget->currentIndex();
        QString currentTitle = ui->tabWidget->tabText(currentIndex);

        if (currentTitle.endsWith("*")) {
            // currentTitle+""; 加空字符并没有改变文件名上面的*
            ui->tabWidget->setTabText(currentIndex, currentTitle.left(currentTitle.length()-1));
        }else{
            // 新建一个空白文本窗口,保存之后该标签上的文件名称并没有被改变
            //qDebug() << currentTitle;
            QFileInfo aFileInfo(aFile);
            QString title=aFileInfo.fileName();
            ui->tabWidget->setTabText(currentIndex,title);
            // 对文本进行修改的时候要加上*
            connect(textEdit, &QPlainTextEdit::textChanged, this, [this, title]() {
                markTabAsModified(title);
            });
        }
    }


    QFileSystemModel* model = qobject_cast<QFileSystemModel*>(ui->treeView->model());
    if (model) {
        // 获取当前目录路径
        QModelIndex currentIndex = ui->treeView->currentIndex();
        if (currentIndex.isValid()) {
            QFileInfo fileInfo = model->fileInfo(currentIndex);
            QString dirPath = fileInfo.isDir() ? fileInfo.absoluteFilePath()
                                               : fileInfo.absolutePath();

            // 刷新指定目录
            QModelIndex dirIndex = model->index(dirPath);
            model->fetchMore(dirIndex); // 重新获取数据
        }
    }

    // // 连接QPlainTextEdit组件的一些常规操作,包括cut, copy, paste
    // connect(textEdit,&QPlainTextEdit::copyAvailable,this,[this,textEdit](bool enabled){
    //     ui->act_Cut->setEnabled(enabled);
    //     ui->act_Copy->setEnabled(enabled);
    //     ui->act_Paste->setEnabled(textEdit->canPaste());
    // });
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    // 获取文件系统模型
    QFileSystemModel* model = qobject_cast<QFileSystemModel*>(ui->treeView->model());
    if (!model) return;

    // 获取文件信息
    QFileInfo fileInfo = model->fileInfo(index);

    // 判断是否是文件（不是文件夹）
    // if (fileInfo.isFile()) {
    //     // 检查文件扩展名，只处理文本文件
    //     QString suffix = fileInfo.suffix().toLower();
    //     QStringList textFileExtensions = {"txt", "cpp", "h", "hpp", "c", "py",
    //                                       "java", "js", "html", "css", "xml",
    //                                       "json", "md", "ini", "cfg", "log"};

    //     if (textFileExtensions.contains(suffix)) {
    //         // 读取文件内容并显示到TabWidget
    //         loadFileToTab(fileInfo.absoluteFilePath());
    //     }
    // }
    if (fileInfo.isFile()) {
        QString suffix = fileInfo.suffix().toLower();
        QString filePath = fileInfo.absoluteFilePath();

        // 文本文件扩展名
        QStringList textFileExtensions = {"txt", "cpp", "h", "hpp", "c", "py",
                                          "java", "js", "html", "css", "xml",
                                          "json", "md", "ini", "cfg", "log"};

        // 图片文件扩展名
        QStringList imageFileExtensions = {"jpg", "jpeg", "png", "bmp",
                                           "gif", "ico", "webp", "tiff"};

        if (textFileExtensions.contains(suffix)) {
            // 读取文件内容并显示到TabWidget
            loadFileToTab(fileInfo.absoluteFilePath());
        }
        else if (imageFileExtensions.contains(suffix)) {
            // 打开图片文件
            openImageFile(filePath);
        }
    }
}

void MainWindow::on_act_SetRoot_triggered()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择目录", QDir::currentPath());
    m_FileSystemModel->setRootPath(dir);
    ui->treeView->setRootIndex(m_FileSystemModel->index(dir));
}


// 关闭单个指定标签页
void MainWindow::on_act_Close_triggered()
{
    int cur=ui->tabWidget->currentIndex();
    ui->tabWidget->removeTab(cur);
}

// 关闭所有
void MainWindow::on_act_CloseAll_triggered()
{
    int totalTab=ui->tabWidget->count();

    QString dlgTitle= "警告！";
    QString strInfo = "当前窗口没有可使用的标签页";
    if(!totalTab) {
        QMessageBox::critical(this, dlgTitle, strInfo);
        return;
    }

    do{
        int cur=ui->tabWidget->currentIndex();
        ui->tabWidget->removeTab(cur);
        totalTab--;
    }while(totalTab);

}

#include <QMenu>
// ai写的
void MainWindow::on_treeView_customContextMenuRequested(const QPoint &pos)
{

    QModelIndex index = ui->treeView->indexAt(pos);
    //QFileSystemModel *model = qobject_cast<QFileSystemModel*>(ui->treeView->model());

    QMenu menu;

    // 如果点击在有效位置上
    if (index.isValid() && m_FileSystemModel) {
        QFileInfo fileInfo = m_FileSystemModel->fileInfo(index);
        QString currentPath;

        if (fileInfo.isDir()) {
            currentPath = fileInfo.absoluteFilePath();
        } else {
            currentPath = fileInfo.absolutePath();
        }

        // 在当前目录下创建文件
        menu.addAction("新建文件", this, [this, currentPath]() {
            createNewFileInDirectory(currentPath);
        });

        menu.addAction("新建文件夹", this, [this, currentPath]() {
            createNewFolderInDirectory(currentPath);
        });

    } else {
        // 点击在空白处，使用模型根目录
        menu.addAction("新建文件", this, [this]() {
            createNewFileInDirectory(m_FileSystemModel->rootPath());
        });

        menu.addAction("新建文件夹", this, [this]() {
            createNewFolderInDirectory(m_FileSystemModel->rootPath());
        });
    }

    menu.exec(ui->treeView->viewport()->mapToGlobal(pos));

}

void MainWindow::on_act_NewDir_triggered()
{
    QModelIndex index=m_FileSystemModel->index(QDir::currentPath());
    m_FileSystemModel->mkdir(index,"新建文件夹");
}

void MainWindow::on_act_Cut_triggered()
{
    QWidget *curWidget=ui->tabWidget->currentWidget();
    QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);

    edit->cut();
}


void MainWindow::on_act_Copy_triggered()
{
    QWidget *curWidget=ui->tabWidget->currentWidget();
    QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);

    edit->copy();
}


void MainWindow::on_act_Paste_triggered()
{
    int numTab=ui->tabWidget->count();
    if(numTab) {
        ui->act_Paste->setEnabled(true);

        QWidget *curWidget=ui->tabWidget->currentWidget();
        QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);

        edit->paste();
    }
    else{
        ui->act_Paste->resetEnabled();
    }
}


void MainWindow::on_act_Undo_triggered()
{
    QWidget *curWidget=ui->tabWidget->currentWidget();
    QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);

    edit->undo();
}


void MainWindow::on_act_Redo_triggered()
{
    QWidget *curWidget=ui->tabWidget->currentWidget();
    QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);

    edit->redo();
}

void MainWindow::on_act_FontBold_triggered(bool checked)
{
    int tabNum=ui->tabWidget->count();
    if(!tabNum) {
        ui->act_FontBold->setChecked(!checked);
        QString dlgTitle= "警告！";
        QString strInfo = "当前窗口没有可使用的标签页";
        QMessageBox::critical(this, dlgTitle, strInfo);
        return;
    }
    QWidget *curWidget=ui->tabWidget->currentWidget();
    QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);
    QTextCharFormat fmt=edit->currentCharFormat();
    if (checked)
        fmt.setFontWeight(QFont::Bold);
    else
        fmt.setFontWeight(QFont::Normal);    //Bold要勾选可选框
    edit->setCurrentCharFormat(fmt);
}

void MainWindow::on_act_FontItalic_triggered(bool checked)
{
    int tabNum=ui->tabWidget->count();
    if(!tabNum) {
        ui->act_FontItalic->setChecked(!checked);
        QString dlgTitle= "警告！";
        QString strInfo = "当前窗口没有可使用的标签页";
        QMessageBox::critical(this, dlgTitle, strInfo);
        return;
    }
    QWidget *curWidget=ui->tabWidget->currentWidget();
    QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);
    QTextCharFormat fmt=edit->currentCharFormat();
    fmt.setFontItalic(checked);        //Italic要勾选可选框
    edit->setCurrentCharFormat(fmt);
}

void MainWindow::on_act_FontUnderline_triggered(bool checked)
{
    int tabNum=ui->tabWidget->count();
    if(!tabNum) {
        ui->act_FontUnderline->setChecked(!checked);
        QString dlgTitle= "警告！";
        QString strInfo = "当前窗口没有可使用的标签页";
        QMessageBox::critical(this, dlgTitle, strInfo);
        return;
    }
    QWidget *curWidget=ui->tabWidget->currentWidget();
    QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);
    QTextCharFormat fmt=edit->currentCharFormat();
    fmt.setFontUnderline(checked);      //Underline要勾选可选框
    edit->setCurrentCharFormat(fmt);
}


void MainWindow::on_act_FontSize_triggered()
{
    int tabNum=ui->tabWidget->count();
    if(!tabNum) {
        QString dlgTitle= "警告！";
        QString strInfo = "当前窗口没有可使用的标签页";
        QMessageBox::critical(this, dlgTitle, strInfo);
        return;
    }
    QWidget *curWidget=ui->tabWidget->currentWidget();
    QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);

    QString dlgTitle= "输入整数对话框";
    QString txtLabel= "设置文本框字体大小";
    int defaultValue= edit->font().pointSize(); //现有字体大小
    int minValue= 5, maxValue= 50, stepValue= 1; //范围、步长
    bool ok= false;
    int inputValue = QInputDialog::getInt(this, dlgTitle,txtLabel,
                                          defaultValue, minValue,maxValue,stepValue,&ok);
    if (ok)
    {
        QFont font= edit->font();
        font.setPointSize(inputValue);
        edit->setFont(font);
    }
}

#include <QFontDialog>
void MainWindow::on_act_FontStyle_triggered()
{
    int tabNum=ui->tabWidget->count();
    if(!tabNum) {
        QString dlgTitle= "警告！";
        QString strInfo = "当前窗口没有可使用的标签页";
        QMessageBox::critical(this, dlgTitle, strInfo);
        return;
    }
    QWidget *curWidget=ui->tabWidget->currentWidget();
    QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(curWidget);

    QFont iniFont= edit->font(); //获取文本框的字体
    bool ok= false; //作为返回值
    QFont font= QFontDialog::getFont(&ok,iniFont); //选择字体
    if (ok) //选择有效
        edit->setFont(font);
}

void MainWindow::on_act_DockVisiable_triggered(bool checked)
{
    ui->dockWidget->setVisible(checked);
}


void MainWindow::on_act_DockFloat_triggered(bool checked)
{
    if(ui->dockWidget->isVisible()){
        ui->dockWidget->setFloating(checked);
    }else{
        ui->dockWidget->setVisible(checked);
        ui->dockWidget->setFloating(checked);
    }
}


void MainWindow::on_dockWidget_visibilityChanged(bool visible)
{
    // 窗口不可视时,浮动也要关闭
    if(visible){
        ui->act_DockVisiable->setChecked(Qt::Checked);
    }else{
        ui->act_DockFloat->setChecked(Qt::Unchecked);
    }
}


void MainWindow::on_dockWidget_topLevelChanged(bool topLevel)
{
    if(topLevel)
        ui->act_DockFloat->setChecked(Qt::Checked);
    else
        ui->act_DockFloat->setChecked(Qt::Unchecked);
}

#include "tformtable.h"
void MainWindow::on_act_LinkDB_triggered()
{
    TFormTable *formTable=new TFormTable(this);
    formTable->setAttribute(Qt::WA_DeleteOnClose);
    int cur=ui->tabWidget->addTab(formTable,
                                    QString::asprintf("Table %d",ui->tabWidget->count()));
    ui->tabWidget->setCurrentIndex(cur);
}

