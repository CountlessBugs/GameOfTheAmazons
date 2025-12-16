#ifndef BOT_H
#define BOT_H

#include <QObject>
#include <QVector>
#include <QSet>
#include <QJsonObject>
#include "chessboard.h"

class Bot : public QObject
{
    Q_OBJECT
public:
    // 所有可能的走法
    struct AllMoves
    {
        // 我方四个棋子的位置
        std::array<QPair<int, int>, 4> positions;
        // 对应棋子的所有可行动范围
        std::array<MoveRange, 4> moves;
        // 所有移动方式总数 (排除被封闭的棋子))
        int moveOpts = 0;
        // 每种方式移动后射箭方式总数 (排除被封闭的棋子))
        QVector<int> shootOpts;
        // 所有行棋方式总数 (排除被封闭的棋子)
        int actionCount = 0;

        // 是否处于官子阶段
        bool isEndgame() const {
            return (moves[0].inClosedRegion() || !moves[0].canMove())
                   && (moves[1].inClosedRegion() || !moves[1].canMove())
                   && (moves[2].inClosedRegion() || !moves[2].canMove())
                   && (moves[3].inClosedRegion() || !moves[3].canMove());
        }
    };

    struct Weights
    {
        // 移动性权重与指数
        double mobilityWeight;
        double mobilityExponent;

        // 射箭灵活性权重与指数
        double shootFlexibilityWeight;
        double shootExponent;

        // 领地面积权重
        double territoryWeight;

        // 中心控制权重
        double centerControlWeight;

        // 棋子分散性权重
        double dispersionWeight;

        // 构造函数初始化列表
        Weights() :
            mobilityWeight(1.0),
            mobilityExponent(0.5),
            shootFlexibilityWeight(0.6),
            shootExponent(0.4),
            territoryWeight(2.0),
            centerControlWeight(0.5),
            dispersionWeight(0.3)
        {}
        Weights(double mobilityW, double mobilityE,
                double shootW, double shootE,
                double territoryW,
                double centerW,
                double dispersionW) :
            mobilityWeight(mobilityW),
            mobilityExponent(mobilityE),
            shootFlexibilityWeight(shootW),
            shootExponent(shootE),
            territoryWeight(territoryW),
            centerControlWeight(centerW),
            dispersionWeight(dispersionW)
        {}

        // 结构体 -> QJsonObject
        QJsonObject toJson() const
        {
            QJsonObject obj;
            obj["mobilityWeight"]          = mobilityWeight;
            obj["mobilityExponent"]        = mobilityExponent;
            obj["shootFlexibilityWeight"]  = shootFlexibilityWeight;
            obj["shootExponent"]           = shootExponent;
            obj["territoryWeight"]         = territoryWeight;
            obj["centerControlWeight"]     = centerControlWeight;
            obj["dispersionWeight"]        = dispersionWeight;
            return obj;
        }

        // QJsonObject -> 结构体
        void fromJson(const QJsonObject &obj)
        {
            mobilityWeight          = obj["mobilityWeight"].toDouble();
            mobilityExponent        = obj["mobilityExponent"].toDouble();
            shootFlexibilityWeight  = obj["shootFlexibilityWeight"].toDouble();
            shootExponent           = obj["shootExponent"].toDouble();
            territoryWeight         = obj["territoryWeight"].toDouble();
            centerControlWeight     = obj["centerControlWeight"].toDouble();
            dispersionWeight        = obj["dispersionWeight"].toDouble();
        }
    };

    // 预设权重
    static const Weights EZ_WEIGHTS;
    static const Weights MD_WEIGHTS;
    static const Weights HD_WEIGHTS;

    explicit Bot(Chessboard *chessboard, bool isWhite = false, Weights weights = Weights(), QObject *parent = nullptr);

    Weights getWeights() const { return m_weights; }

public slots:
    // 下一步棋
    bool makeNextMove();

    // 重置AI状态
    void reset();

private:
    Chessboard *m_chessboard;
    Chessboard *m_sandbox; // 沙盒棋盘，用于模拟走法

    bool m_isWhite; // 是否为白方AI
    bool m_gameOver; // 游戏是否结束

    // 获取所有可能走法
    AllMoves getAllMoves(bool inSandbox = false) const;
    // 获取指定阵营的所有可能走法
    AllMoves getAllMovesForSide(bool isWhite, bool inSandbox = false) const;
    // 执行一步走法
    bool makeMove(const Move &move);

    // 官子阶段
    bool m_endgame; // 是否处于官子阶段
    QVector<Move> m_endgameMoves; // 在官子阶段记录当前棋子的行棋策略
    QSet<Chessboard::Board> m_endgameVisited; // 记录官子阶段已访问的棋盘状态

    QVector<Move> getEndgameMoves(QPair<int, int> piecePos = {-1, -1}); // 获取官子阶段一个棋子的行棋策略
    bool makeMoveInEndgame(); // 官子阶段行棋

    bool makeMoveInSandbox(const Move &move, bool skipOpponentTurn = false, bool autoSetTurnState = false); // 在沙盒棋盘上执行一步走法
    bool reverseMoveInSandbox(const Move &move); // 撤销沙盒棋盘上的一步走法
    void resetSandbox(); // 重置沙盒棋盘为当前的真实棋盘状态

    // 行棋算法
    // Minimax算法
    double evalSandbox(); // 计算当前沙盒棋盘评分
    Move getBestMove(int depth = 0); // 获取最佳走法
    // Alpha-Beta 搜索函数
    // alpha: 当前层最大化玩家已找到的最好值
    // beta: 当前层最小化玩家已找到的最好值
    double alphaBeta(int depth, double alpha, double beta, bool maximizingPlayer);
    // 直接生成所有合法走法列表 (比 getMoveByIndex 更快)
    QVector<Move> generateLegalMoves(bool forWhite, bool inSandbox) const;

    // 评估函数权重
    Weights m_weights;
};

#endif // BOT_H
