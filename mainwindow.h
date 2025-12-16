#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>

#include "chessboard.h"
#include "chessboardwidget.h"
#include "bot.h"
#include "savegame.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // UI 核心组件
    QStackedWidget *m_stackedWidget; // 页面管理器
    QWidget *m_gamePageContainer; // 游戏页面的容器
    QVBoxLayout *m_gamePageLayout; // 游戏页面的布局

    // 游戏页面控件
    QPushButton *m_btnReplay;

    // 设置页面控件
    QComboBox *m_comboWhite;
    QComboBox *m_comboBlack;

    // 读取存档页面控件
    QListWidget *m_saveFileList;

    // 核心逻辑成员
    Chessboard *m_chessboard;
    ChessboardWidget *m_chessboardWidget;
    Bot *m_whiteBot;
    Bot *m_blackBot;
    SaveGame *m_saveGame;
    bool m_gameSaved; // 当前游戏是否已被保存，用于返回主菜单时提示

    // 核心逻辑函数
    void initChessboard(bool whiteIsPlayer, bool blackIsPlayer);
    void initBots(bool initWhiteBot, bool initBlackBot, int difficultyWhite = 1, int difficultyBlack = 1);
    bool saveGameAsJson();

    // UI 初始化函数
    void setupUI(); // 总布局初始化
    QWidget* createMainMenu(); // 创建主菜单页
    QWidget* createSetupPage(); // 创建选人设置页
    QWidget* createGamePage(); // 创建游戏容器页
    QWidget* createLoadPage(); // 创建读取存档页

    // 刷新存档列表
    void refreshSaveList();

    // 辅助函数：判断下拉框选的是不是玩家
    bool isPlayer(QComboBox *combo);
};
#endif // MAINWINDOW_H
