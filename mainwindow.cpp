#include "mainwindow.h"
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QDir>
#include <QMessageBox>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chessboard{nullptr}
    , m_chessboardWidget{nullptr}
    , m_whiteBot{nullptr}
    , m_blackBot{nullptr}
    , m_saveGame{nullptr}
    , m_gameSaved{true}
{
    // 初始化 UI
    setupUI();
}

MainWindow::~MainWindow()
{
    if (m_chessboard) delete m_chessboard;
    if (m_chessboardWidget) delete m_chessboardWidget;
    if (m_whiteBot) delete m_whiteBot;
    if (m_blackBot) delete m_blackBot;
    if (m_saveGame) delete m_saveGame;
}

void MainWindow::setupUI()
{
    this->setWindowTitle("Game Of The Amazons");
    this->resize(1280, 800);

    this->setStyleSheet(
        "QMainWindow { border-image: url(:/images/pieces/res/bg.jpg) 0 0 0 0 stretch stretch; }"
        );

    // 创建堆叠式页面管理器
    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setStyleSheet("background: transparent;");
    setCentralWidget(m_stackedWidget);

    // 添加页面
    // Index 0: 主菜单
    m_stackedWidget->addWidget(createMainMenu());
    // Index 1: 设置页面
    m_stackedWidget->addWidget(createSetupPage());
    // Index 2: 游戏页面
    m_stackedWidget->addWidget(createGamePage());
    // Index 3: 读取存档页面
    m_stackedWidget->addWidget(createLoadPage());

    // 默认显示主菜单
    m_stackedWidget->setCurrentIndex(0);
}

QWidget* MainWindow::createMainMenu()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    // 创建半透明圆角背景容器
    QWidget *container = new QWidget(page);
    container->setStyleSheet(
        "background-color: rgba(0, 0, 0, 0.5);"  // 半透明黑色背景
        "border-radius: 20px;"                  // 圆角边框
        "padding: 40px;"                        // 内边距
        );

    // 在容器内创建垂直布局
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setAlignment(Qt::AlignCenter);
    containerLayout->setSpacing(30); // 控件间距

    QLabel *title = new QLabel("亚马逊棋\nGame of the Amazons", page);
    title->setStyleSheet(
        "font-size: 60px; "        // 字号加大
        "font-weight: bold; "
        "font-family: 'Microsoft YaHei'; "
        "color: #FFFFFF;"          // 白色文字
        "margin-bottom: 20px;"
        "background: transparent;" // 确保标题背景透明
        );
    title->setAlignment(Qt::AlignCenter);

    QPushButton *btnStart = new QPushButton("开始新游戏", page);
    QPushButton *btnLoad = new QPushButton("读取存档", page);

    QString btnStyle =
        "QPushButton { "
        "   padding: 15px 30px; "      // 内边距加大
        "   font-size: 24px; "         // 字体加大
        "   font-weight: bold;"
        "   border-radius: 15px; "     // 圆角
        "   background-color: rgba(255, 255, 255, 0.9); "
        "   color: #333; "
        "   min-width: 300px; "        // 最小宽度加大
        "   min-height: 60px;"         // 最小高度加大
        "} "
        "QPushButton:hover { background-color: #FFFFFF; } " // 悬停变亮
        "QPushButton:pressed { background-color: #DDDDDD; }";

    btnStart->setStyleSheet(btnStyle);
    btnLoad->setStyleSheet(btnStyle);

    // 将控件添加到容器布局中
    containerLayout->addStretch(1);
    containerLayout->addWidget(title);
    containerLayout->addSpacing(20);
    containerLayout->addWidget(btnStart);
    containerLayout->addWidget(btnLoad);
    containerLayout->addStretch(1);

    // 将容器添加到主布局
    layout->addStretch(1);
    layout->addWidget(container);  // 放在弹簧之间居中
    layout->addStretch(1);

    // --- 信号连接 ---

    // 点击开始 -> 跳转到设置页
    connect(btnStart, &QPushButton::clicked, this, [this]() {
        m_stackedWidget->setCurrentIndex(1);
    });

    // 点击读取 -> 跳转到存档列表页 (Index 3) 并刷新列表
    connect(btnLoad, &QPushButton::clicked, this, [this]() {
        refreshSaveList(); // 每次进入页面前刷新
        m_stackedWidget->setCurrentIndex(3);
    });

    return page;
}

QWidget* MainWindow::createSetupPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    // 创建半透明圆角背景容器
    QWidget *container = new QWidget(page);
    container->setObjectName("SetupContainer");

    // 在容器内创建垂直布局
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setAlignment(Qt::AlignCenter);
    containerLayout->setSpacing(30);

    QLabel *label = new QLabel("游戏设置", page);
    label->setStyleSheet(
        "font-size: 40px; font-weight: bold; margin-bottom: 30px; color: white;"
        "background: transparent;"
        );
    label->setAlignment(Qt::AlignCenter);

    // 统一定义样式
    QString commonStyle =
        // 针对容器本身的样式 (使用 #ID 选择器)
        "QWidget#SetupContainer { "
        "   background-color: rgba(0, 0, 0, 0.5);"
        "   border-radius: 20px;"
        "   padding: 40px;"
        "}"
        // GroupBox 样式
        "QGroupBox { "
        "   font-size: 20px; font-weight: bold; color: white; "
        "   border: 2px solid rgba(255,255,255,0.6); border-radius: 10px; margin-top: 10px; padding-top: 20px; "
        "   background-color: transparent;"
        "}"
        "QGroupBox::title { subcontrol-origin: margin; left: 15px; padding: 0 5px; }"
        // ComboBox 样式
        "QComboBox { "
        "   font-size: 18px; padding: 5px 10px; border-radius: 5px; min-width: 200px; "
        "   background-color: rgba(255,255,255,0.95); color: #000000;"
        "   border: 1px solid #ccc;"
        "}"
        "QComboBox::drop-down { border: 0px; width: 30px; }"
        "QComboBox::down-arrow { image: none; border-left: 2px solid #666; width: 0; height: 0; }"
        // 下拉列表的样式
        "QComboBox QAbstractItemView { "
        "   background-color: white; color: black; selection-background-color: #4CAF50; selection-color: white;"
        "   border: 1px solid gray;"
        "}"
        "QPushButton { "
        "   font-size: 20px; padding: 12px 24px; border-radius: 8px; font-weight: bold;"
        "   min-width: 120px;"
        "}";

    container->setStyleSheet(commonStyle);

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->setSpacing(40); // 加大左右分栏间距

    // 白方
    QGroupBox *grpWhite = new QGroupBox("白方 (先手)", page);
    QVBoxLayout *vWhite = new QVBoxLayout(grpWhite);
    m_comboWhite = new QComboBox(page);
    m_comboWhite->addItems({"玩家", "Bot (简单)", "Bot (中等)", "Bot (困难)"});
    vWhite->addWidget(m_comboWhite);

    // 黑方
    QGroupBox *grpBlack = new QGroupBox("黑方 (后手)", page);
    QVBoxLayout *vBlack = new QVBoxLayout(grpBlack);
    m_comboBlack = new QComboBox(page);
    m_comboBlack->addItems({"玩家", "Bot (简单)", "Bot (中等)", "Bot (困难)"});
    // 默认黑方选 Bot
    m_comboBlack->setCurrentIndex(1);
    vBlack->addWidget(m_comboBlack);

    hLayout->addWidget(grpWhite);
    hLayout->addWidget(grpBlack);

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);

    QPushButton *btnBack = new QPushButton("返回", page);
    btnBack->setStyleSheet("background-color: rgba(255, 255, 255, 0.8); color: #333;");

    QPushButton *btnConfirm = new QPushButton("进入游戏", page);
    btnConfirm->setStyleSheet("background-color: #4CAF50; color: white; border: none;"); // 绿色按钮

    btnLayout->addWidget(btnBack);
    btnLayout->addWidget(btnConfirm);

    // 将所有控件添加到容器布局中
    containerLayout->addStretch(1);
    containerLayout->addWidget(label);
    containerLayout->addLayout(hLayout);
    containerLayout->addSpacing(50);
    containerLayout->addLayout(btnLayout);
    containerLayout->addStretch(1);

    // 将容器添加到主布局并居中
    layout->addStretch(1);
    layout->addWidget(container);
    layout->addStretch(1);

    // 信号连接

    connect(btnBack, &QPushButton::clicked, this, [this](){
        m_stackedWidget->setCurrentIndex(0);
    });

    connect(btnConfirm, &QPushButton::clicked, this, [this](){
        // 判断白方是否为玩家
        bool whiteIsPlayer = isPlayer(m_comboWhite);
        bool blackIsPlayer = isPlayer(m_comboBlack);

        // 调用你的核心初始化函数
        initChessboard(whiteIsPlayer, blackIsPlayer);

        // 初始化 Bot (如果是 Player 则为 false, 是 Bot 则为 true)
        initBots(!whiteIsPlayer, !blackIsPlayer, m_comboWhite->currentIndex(), m_comboBlack->currentIndex());

        // 初始化存档
        if(m_saveGame) delete m_saveGame;
        m_saveGame = new SaveGame(m_chessboard, m_whiteBot, m_blackBot);

        // 跳转页面
        m_stackedWidget->setCurrentIndex(2);
    });

    return page;
}

QWidget* MainWindow::createGamePage()
{
    // 这是一个容器，稍后 initChessboard 会把棋盘 Widget 加进来
    m_gamePageContainer = new QWidget(this);
    m_gamePageLayout = new QVBoxLayout(m_gamePageContainer);

    // 设置样式
    m_gamePageContainer->setStyleSheet(
        "QPushButton {"
        "    background-color: white;"
        "    color: #2c3e50;"
        "    border: 1px solid #bdc3c7;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    min-width: 100px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #f8f9fa;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #e9ecef;"
        "    border: 1px solid #95a5a6;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #e0e0e0;"
        "    color: #9e9e9e;"
        "    border: 1px solid #d0d0d0;"
        "}"
        );

    // 顶部工具栏（退出/保存/回放按钮）
    QHBoxLayout *toolBar = new QHBoxLayout();
    QPushButton *btnExit = new QPushButton("返回菜单", m_gamePageContainer);
    QPushButton *btnSave = new QPushButton("保存游戏", m_gamePageContainer);
    m_btnReplay = new QPushButton("回放", m_gamePageContainer);

    // 初始禁用回放按钮，游戏结束后启用
    m_btnReplay->setDisabled(true);

    toolBar->addWidget(btnExit);
    toolBar->addWidget(btnSave);
    toolBar->addWidget(m_btnReplay);
    toolBar->addStretch();

    m_gamePageLayout->addLayout(toolBar);
    // 棋盘稍后会通过 addWidget 添加到这里

    // 连接信号
    connect(btnSave, &QPushButton::clicked, this, [this](){
        bool gameSaved = saveGameAsJson();
        if (gameSaved) {
            QMessageBox::information(this, "保存成功", "游戏已成功保存");
        } else {
            QMessageBox::warning(this, "保存失败", "游戏保存失败，请重试");
        }
    });
    connect(btnExit, &QPushButton::clicked, this, [this](){
        // 若游戏已保存，已结束或正在回放则不弹出提示框
        if (m_gameSaved
            || m_chessboard->checkGameOver()
            || m_chessboard->isReplaying()) {

            m_stackedWidget->setCurrentIndex(0);
            return;
        }

        // 弹出确认对话框
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,
                        tr("确认退出"),
                        tr("确定要返回到主菜单吗？当前游戏不会被自动保存"),
                        QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            // 用户点击"是"，返回主菜单
            m_stackedWidget->setCurrentIndex(0);
        }
    });
    connect(m_btnReplay, &QPushButton::clicked, this, [this](){
        if(m_chessboardWidget) {
            m_chessboard->startReplay();
        }
    });

    return m_gamePageContainer;
}

QWidget* MainWindow::createLoadPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    QWidget *container = new QWidget(page);
    container->setObjectName("LoadContainer"); // 方便设置样式

    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(20);

    QLabel *label = new QLabel("选择存档", page);
    label->setStyleSheet("font-size: 40px; font-weight: bold; margin-bottom: 20px; color: white; background: transparent;");
    label->setAlignment(Qt::AlignCenter);

    // 初始化 ListWidget
    m_saveFileList = new QListWidget(page);

    // 设置样式
    QString commonStyle =
        "QWidget#LoadContainer { "
        "   background-color: rgba(0, 0, 0, 0.6);"
        "   border-radius: 20px;"
        "   padding: 40px;"
        "}"
        "QListWidget { "
        "   background-color: rgba(255, 255, 255, 0.9);"
        "   border-radius: 10px;"
        "   font-size: 18px;"
        "   color: #333;"
        "   padding: 10px;"
        "   outline: none;" // 去除选中虚线框
        "}"
        "QListWidget::item { padding: 10px; border-bottom: 1px solid #ddd; }"
        "QListWidget::item:selected { background-color: #4CAF50; color: white; border-radius: 5px; }"
        "QPushButton { "
        "   font-size: 18px; padding: 10px 20px; border-radius: 8px; font-weight: bold;"
        "   min-width: 100px;"
        "}";

    container->setStyleSheet(commonStyle);

    // 按钮组
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);

    QPushButton *btnBack = new QPushButton("返回", page);
    btnBack->setStyleSheet("background-color: rgba(255, 255, 255, 0.8); color: #333;");

    QPushButton *btnDelete = new QPushButton("删除", page);
    btnDelete->setStyleSheet("background-color: #E74C3C; color: white; border: none;"); // 红色

    QPushButton *btnLoadConfirm = new QPushButton("加载游戏", page);
    btnLoadConfirm->setStyleSheet("background-color: #4CAF50; color: white; border: none;"); // 绿色

    btnLayout->addWidget(btnBack);
    btnLayout->addWidget(btnDelete);
    btnLayout->addWidget(btnLoadConfirm);

    containerLayout->addWidget(label);
    containerLayout->addWidget(m_saveFileList);
    containerLayout->addLayout(btnLayout);

    // 设置固定大小或比例，防止列表太小
    m_saveFileList->setMinimumSize(500, 400);

    layout->addStretch(1);
    layout->addWidget(container);
    layout->addStretch(1);

    // 信号连接

    // 返回
    connect(btnBack, &QPushButton::clicked, this, [this](){
        m_stackedWidget->setCurrentIndex(0);
    });

    // 删除存档
    connect(btnDelete, &QPushButton::clicked, this, [this](){
        QListWidgetItem *item = m_saveFileList->currentItem();
        if (!item) {
            QMessageBox::warning(this, "未选择存档", "请先选择一个存档");
            return;
        }

        QString fileName = item->text();
        QString filePath = "./saves/" + fileName;

        // 创建删除存档对话框
        QMessageBox msgBox(QMessageBox::Question, "确认删除",
                           QString("确定要删除存档 %1 吗？").arg(fileName),
                           QMessageBox::Yes | QMessageBox::No, this);

        // 执行对话框并获取结果
        int ret = msgBox.exec();

        if (ret == QMessageBox::Yes) {
            QFile file(filePath);
            if (file.remove()) {
                refreshSaveList(); // 删除成功后刷新列表
            } else {
                QMessageBox::warning(this, "删除失败", "存档删除失败");
            }
        }
    });

    // 确认加载
    connect(btnLoadConfirm, &QPushButton::clicked, this, [this](){
        QListWidgetItem *item = m_saveFileList->currentItem();
        if (!item) {
            QMessageBox::warning(this, "未选择存档", "请先选择一个存档");
            return;
        }

        QString fileName = item->text();
        QString savePath = "./saves/" + fileName;

        // SaveGame 加载时需要先初始化一个空白棋盘
        initChessboard(true, true);

        if(m_saveGame) delete m_saveGame;
        m_saveGame = new SaveGame(m_chessboard);

        if (m_saveGame->loadSave(savePath)) {
            // 获取 Bot
            m_whiteBot = m_saveGame->getBots().first;
            m_blackBot = m_saveGame->getBots().second;
            // 加载成功跳转
            m_stackedWidget->setCurrentIndex(2);
        } else {
            QMessageBox::warning(this, "读取失败", "读取存档文件失败");
        }
    });

    return page;
}

void MainWindow::refreshSaveList()
{
    if (!m_saveFileList) return;

    m_saveFileList->clear();

    QDir dir("./saves");
    if (!dir.exists()) {
        dir.mkpath("."); // 如果目录不存在则创建
    }

    // 获取所有 json 文件，按修改时间倒序排列（最新的在最上面）
    QFileInfoList fileList = dir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Time);

    for (const QFileInfo &fileInfo : std::as_const(fileList)) {
        // 显示文件名
        m_saveFileList->addItem(fileInfo.fileName());
    }

    // 如果有文件，默认选中第一个
    if (m_saveFileList->count() > 0) {
        m_saveFileList->setCurrentRow(0);
    }
}

bool MainWindow::isPlayer(QComboBox *combo) {
    // 假设索引 0 是 "玩家 (Player)"
    return combo->currentIndex() == 0;
}

void MainWindow::initChessboard(bool whiteIsPlayer, bool blackIsPlayer)
{
    // 清理旧的游戏资源
    if(m_chessboardWidget) {
        m_gamePageLayout->removeWidget(m_chessboardWidget);
        delete m_chessboardWidget; m_chessboardWidget = nullptr;
    }
    if(m_chessboard) { delete m_chessboard; m_chessboard = nullptr; }

    // 创建逻辑棋盘
    m_chessboard = new Chessboard(whiteIsPlayer, blackIsPlayer, true, this);

    // 创建显示控件
    m_chessboardWidget = new ChessboardWidget(m_chessboard, this);

    // 将棋盘控件添加到游戏页面布局中
    m_gamePageLayout->addWidget(m_chessboardWidget);

    // 连接棋盘相关信号
    // 行棋后更新存档状态为未保存
    connect(m_chessboard, &Chessboard::moveMade, this, [this]() {
        m_gameSaved = false;
    });
    // 游戏结束时启用回放按钮
    connect(m_chessboard, &Chessboard::gameOver, this, [this]() {
        m_btnReplay->setDisabled(false);
    });
    // 回放时禁用回放按钮
    connect(m_chessboard, &Chessboard::replayStarted, this, [this]() {
        m_btnReplay->setDisabled(true);
    });
    // 回放结束启用回放按钮
    connect(m_chessboard, &Chessboard::replayFinished, this, [this]() {
        m_btnReplay->setDisabled(false);
    });
}

void MainWindow::initBots(bool initWhiteBot, bool initBlackBot, int difficultyWhite, int difficultyBlack)
{
    // 清理旧的 Bot 实例
    if(m_whiteBot) { delete m_whiteBot; m_whiteBot = nullptr; }
    if(m_blackBot) { delete m_blackBot; m_blackBot = nullptr; }

    if (initWhiteBot) {
        switch (difficultyWhite) {
        case 1:
            m_whiteBot = new Bot(m_chessboard, true, Bot::EZ_WEIGHTS, this);
            break;
        case 2:
            m_whiteBot = new Bot(m_chessboard, true, Bot::MD_WEIGHTS, this);
            break;
        case 3:
            m_whiteBot = new Bot(m_chessboard, true, Bot::HD_WEIGHTS, this);
            break;
        default:
            m_whiteBot = new Bot(m_chessboard, true, Bot::Weights(), this);
            break;
        }

    }
    if (initBlackBot) {
        switch (difficultyBlack) {
        case 1:
            m_blackBot = new Bot(m_chessboard, false, Bot::EZ_WEIGHTS, this);
            break;
        case 2:
            m_blackBot = new Bot(m_chessboard, false, Bot::MD_WEIGHTS, this);
            break;
        case 3:
            m_blackBot = new Bot(m_chessboard, false, Bot::HD_WEIGHTS, this);
            break;
        default:
            m_blackBot = new Bot(m_chessboard, false, Bot::Weights(), this);
            break;
        }
    }
}

bool MainWindow::saveGameAsJson()
{
    if(m_saveGame) {
        if (m_saveGame->saveGameAsJson()) {
            m_gameSaved = true;
            return true;
        }
    }
    return false;
}
