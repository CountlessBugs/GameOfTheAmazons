#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include <QObject>
#include <QTimer>
#include <array>

// 棋盘上某位置棋子可移动范围的数据结构
struct MoveRange {
    int up = 0;
    int down = 0;
    int left = 0;
    int right = 0;
    int upLeft = 0;
    int upRight = 0;
    int downLeft = 0;
    int downRight = 0;
    int territoryArea = -1; // -1代表未处于官子阶段，否则为官子阶段领地大小

    // 获取走法总数
    int getTotalMoves() const {
        return up + down + left + right
               + upLeft + upRight + downLeft + downRight;
    }

    bool canMove() const {
        return getTotalMoves() > 0;
    }

    bool inClosedRegion() const {
        return territoryArea >= 0;
    }
};

// 一步行棋的数据结构
struct Move
{
    // 原始位置
    QPair<int, int> startPos = {-1, -1};
    // 目标位置
    QPair<int, int> targetPos = {-1, -1};
    // 射箭位置
    QPair<int, int> shootPos = {-1, -1};

    // 显式声明默认构造函数和拷贝赋值，让 Clazy 闭嘴
    Move() = default;
    Move(const Move&) = default;
    Move& operator=(const Move&) = default;
};

class Chessboard : public QObject
{
    Q_OBJECT
public:
    // 棋盘格子的枚举类
    enum class Cell : quint8 {
        Empty  = 0,
        Block  = 1,
        White  = 2,
        Black  = 3
    };
    Q_ENUM(Cell)

    // 回合状态的枚举类
    enum class TurnState : quint8 {
        WhiteMove  = 0,
        WhiteShoot = 1,
        BlackMove  = 2,
        BlackShoot = 3
    };
    Q_ENUM(TurnState)

    // 游戏获胜者的枚举类
    enum class Winner : quint8 {
        White = 0,
        Black = 1,
        Tie = 2
    };
    Q_ENUM(Winner)

public:
    explicit Chessboard(bool whiteIsPlayer = true, bool blackIsPlayer = false, bool autoCheckGameOver = true, QObject *parent = nullptr);

private:
    friend class Bot; // 允许Bot访问私有成员

public:
    // 棋盘布局类型
    using Board = std::array<std::array<Chessboard::Cell, 8>, 8>;

    // 行棋状态
    Board m_board;
    QPair<int, int> m_selected; // 当前选中的棋子行列索引, -1代表未选中
    TurnState m_turnState;
    QVector<Move> m_history; // 行棋历史记录

    // 玩家身份
    bool m_whiteIsPlayer;
    bool m_blackIsPlayer;

    bool m_autoCheckGameOver;

    // 行棋操作
    // 行棋一步
    bool makeMove(const Move &move);
    // 尝试清除选中的棋子, 射箭阶段不可清除
    bool tryToClearSelected();
    // 尝试选中棋子, 射箭阶段不可选中其他棋子
    bool tryToSelect(int row, int col);
    // 移动, 射箭操作
    bool moveSelectedTo(int row, int col);
    bool shootAt(int row, int col);

    // 检查游戏是否结束
    bool checkGameOver();

    // 回放功能
    void startReplay(); // 开始回放
    void stopReplay();  // 停止回放
    QVector<Move> getReplayHistory() const { return m_replayHistory; }
    bool isReplaying() const { return m_isReplaying; }
    int getReplayStep() const { return m_replayStep; }
    int getReplayTotalSteps() const { return m_replayHistory.size(); }

private:
    // 判断是否能移动或射箭到目标位置
    bool pathValid(int startRow, int startCol, int endRow, int endCol) const;

    // 游戏结束处理
    void onGameOver(Winner winner);

    // 行棋状态
    Move m_currentMove;
    bool m_gameOver; // 游戏是否结束

    // 回放相关
    bool m_isReplaying;           // 是否正在回放
    int m_replayStep;             // 当前回放到第几步
    QVector<Move> m_replayHistory; // 回放使用的历史记录副本
    QTimer *m_replayTimer;        // 回放定时器

public:
    // 获取处于官子阶段的指定棋子领地大小(非官子阶段为-1)
    int getTerritoryArea(int row, int col, bool isWhite, Board *board = nullptr) const;
    // 获取指定棋子活动范围
    MoveRange getMoveRange(int row, int col) const;
    // 获取指定棋子活动范围, 忽略某个位置的阻挡
    MoveRange getMoveRangeIgnoring(int row, int col, int ignoreRow, int ignoreCol);
    // 获取指定棋子是否可以移动
    bool canMove(int row, int col) const;
    // 重置棋盘, 回到初始状态
    void reset();

signals:
    // 行棋的信号
    void moveMade(Move move, bool isWhite);
    // 加载棋盘的信号
    void boardLoaded();
    // 游戏结束的信号
    void gameOver(Chessboard::Winner winner);

    // 回放相关
    void replayStarted();
    void replayStep(int currentStep, int totalSteps);
    void replayFinished();

private slots:
    void onReplayTimerTimeout(); // 回放定时器槽函数

};

// 使Cell可哈希
inline bool operator==(Chessboard::Cell a, Chessboard::Cell b)
{ return static_cast<quint8>(a) == static_cast<quint8>(b); }

inline uint qHash(Chessboard::Cell c, uint seed = 0)
{ return qHash(static_cast<quint8>(c), seed); }

// 使Board可哈希
inline bool operator==(const Chessboard::Board &a,
                       const Chessboard::Board &b)
{
    return std::memcmp(a.data(), b.data(), sizeof(a)) == 0;
}

inline size_t qHash(const Chessboard::Board &b, size_t seed = 0)
{
    static_assert(sizeof(Chessboard::Board) == 64);
    const auto *p = reinterpret_cast<const quint64*>(b.data());
    return qHashBits(p, sizeof(Chessboard::Board), seed);
}

#endif // CHESSBOARD_H
