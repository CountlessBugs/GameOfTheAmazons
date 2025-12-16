#ifndef CHESSBOARDWIDGET_H
#define CHESSBOARDWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QVariantAnimation>
#include "chessboard.h"

// TODO: 添加游戏结束处理

class ChessboardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChessboardWidget(Chessboard *chessboard, QWidget *parent = nullptr);

    float getAnimDuration() const { return m_animDuration; }
    void setAnimDuration(float duration) { m_animDuration = duration; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

    Chessboard *m_chessboard;

private slots:
    // 动画槽函数
    void onAnimValueChanged(const QVariant &value);
    void onAnimFinished();
    void onMoveMade(const Move &move, bool isWhite);
    void onGameOver(Chessboard::Winner winner);

    // 回放槽函数
    void onReplayStarted();
    void onReplayStep(int currentStep, int totalSteps);
    void onReplayFinished();

private:
    // 外观
    // 棋盘颜色
    QColor m_lightColor;
    QColor m_darkColor;
    QColor m_moveRangeLightColor; // 移动范围内的浅色格子
    QColor m_moveRangeDarkColor; // 移动范围内的深色格子
    // 棋子图片
    QPixmap m_blackPiecePixmap;
    QPixmap m_whitePiecePixmap;
    QPixmap m_blockPiecePixmap;
    QPixmap m_arrowPixmap;
    // 棋盘布局(实时更新)
    int m_boardSize; // 棋盘实际大小(正方形)
    int m_cellSize; // 每个格子的大小
    int m_marginX; // 水平边距
    int m_marginY; // 垂直边距

    // 动画
    QVariantAnimation *m_animation; // 动画对象
    QVariantAnimation *m_celebrationAnim; // 庆祝动画对象

    enum AnimationType {
        None,
        Moving, // 棋子移动
        Shooting, // 射箭
        Celebrating // 战胜庆祝
    };
    float m_animDuration; // 动画时长 (毫秒)
    AnimationType m_animType; // 当前动画类型
    QPoint m_animStartPos; // 动画起始网格坐标 (row, col)
    QPoint m_animEndPos; // 动画结束网格坐标 (row, col)
    qreal m_animProgress; // 动画进度 (0.0 - 1.0)
    Move m_currentAnimMove; // 当前动画对应的走法

    // 游戏结束状态
    bool m_gameOver; // 游戏是否结束
    Chessboard::Winner m_winner; // 获胜方

    // 辅助函数：将网格坐标转换为像素中心坐标
    QPointF getCenterPos(int row, int col) const;
};

#endif // CHESSBOARDWIDGET_H
