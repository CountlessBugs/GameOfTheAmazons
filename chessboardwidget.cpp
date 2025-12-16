#include "chessboardwidget.h"
#include <QPainter>
#include <QMouseEvent>

ChessboardWidget::ChessboardWidget(Chessboard *chessboard, QWidget *parent)
    : QWidget{parent}
    , m_chessboard{chessboard}
    , m_animType(None)
    , m_animDuration(300)
    , m_animProgress(0.0)
    , m_gameOver(false)
    , m_winner(Chessboard::Winner::Tie)
{
    // 加载图片
    m_blackPiecePixmap.load(":/images/pieces/res/black.png");
    m_whitePiecePixmap.load(":/images/pieces/res/white.png");
    m_blockPiecePixmap.load(":/images/pieces/res/block.png");
    m_arrowPixmap.load(":/images/pieces/res/arrow.png");

    // 初始化动画对象
    m_animation = new QVariantAnimation(this);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setDuration(m_animDuration);
    m_animation->setEasingCurve(QEasingCurve::OutQuad); // 减速曲线，使动作更自然
    connect(m_animation, &QVariantAnimation::valueChanged, this, &ChessboardWidget::onAnimValueChanged);
    connect(m_animation, &QVariantAnimation::finished, this, &ChessboardWidget::onAnimFinished);

    // 初始化庆祝动画（循环跳动）
    m_celebrationAnim = new QVariantAnimation(this);
    m_celebrationAnim->setStartValue(0.0);
    m_celebrationAnim->setEndValue(1.0);
    m_celebrationAnim->setDuration(600); // 跳动周期
    m_celebrationAnim->setLoopCount(-1); // 无限循环
    m_celebrationAnim->setEasingCurve(QEasingCurve::InOutSine); // 正弦曲线，模拟跳动
    connect(m_celebrationAnim, &QVariantAnimation::valueChanged, this, [this]() {
        update(); // 触发重绘
    });

    // 加载初始棋盘
    m_lightColor = QColor(240, 217, 181);           // 浅米色
    m_darkColor = QColor(181, 136, 99);             // 深棕色
    m_moveRangeLightColor = QColor(245, 235, 180);  // 淡黄色
    m_moveRangeDarkColor = QColor(240, 220, 150);   // 浅黄色

    // 连接信号
    // 落子后触发动画
    connect(m_chessboard, &Chessboard::moveMade, this, &ChessboardWidget::onMoveMade);
    // 游戏结束信号
    connect(m_chessboard, &Chessboard::gameOver, this, &ChessboardWidget::onGameOver);
    // 回放信号
    connect(m_chessboard, &Chessboard::replayStarted, this, &ChessboardWidget::onReplayStarted);
    connect(m_chessboard, &Chessboard::replayStep, this, &ChessboardWidget::onReplayStep);
    connect(m_chessboard, &Chessboard::replayFinished, this, &ChessboardWidget::onReplayFinished);
    connect(m_chessboard, &Chessboard::boardLoaded, this, [this]() { update(); });
}

void ChessboardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true); // 图片平滑变换

    // 计算棋盘尺寸
    m_boardSize = qMin(width(), height()); // 棋盘实际大小(正方形)
    m_cellSize = m_boardSize / 8; // 每个格子的大小
    // 调整棋盘大小，使其为8的倍数，避免像素不对齐
    m_boardSize = m_cellSize * 8;
    m_marginX = (width() - m_boardSize) / 2; // 水平边距
    m_marginY = (height() - m_boardSize) / 2; // 垂直边距

    // 绘制背景
    // 绘制深浅交替的格子, 并标明移动范围
    QPair<int, int> selected = m_chessboard->m_selected;
    MoveRange moveRange = m_chessboard->getMoveRange(selected.first, selected.second);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            QRect rect(m_marginX + col * m_cellSize,
                       m_marginY + row * m_cellSize,
                       m_cellSize, m_cellSize);

            // 判断是否在移动范围内
            bool inMoveRange = false;
            if (row == selected.first) {
                if (col >= selected.second - moveRange.left
                    && col <= selected.second + moveRange.right) {
                    inMoveRange = true;
                }
            } else if (col == selected.second) {
                if (row >= selected.first - moveRange.up
                    && row <= selected.first + moveRange.down) {
                    inMoveRange = true;
                }
            } else if (row - col == selected.first - selected.second) {
                if (row >= selected.first - moveRange.upLeft
                    && row <= selected.first + moveRange.downRight) {
                    inMoveRange = true;
                }
            } else if (row + col == selected.first + selected.second) {
                if (row >= selected.first - moveRange.upRight
                    && row <= selected.first + moveRange.downLeft) {
                    inMoveRange = true;
                }
            }

            // 判断格子颜色：行列索引和为偶数是浅色，奇数是深色
            if ((row + col) % 2 == 0) {
                painter.fillRect(rect, (inMoveRange ? m_moveRangeLightColor : m_lightColor));
            } else {
                painter.fillRect(rect, (inMoveRange ? m_moveRangeDarkColor : m_darkColor));
            }

            // 绘制格子边框
            painter.setPen(Qt::black);
            painter.drawRect(rect);
        }
    }

    // 绘制棋子
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            // 动画处理：如果当前格子是动画的目标位置或者射箭位置，则暂时跳过绘制
            if (m_animType != None) {
                if (row == m_animEndPos.x() && col == m_animEndPos.y()
                    || (m_animType == Moving
                        && row == m_currentAnimMove.shootPos.first
                        && col == m_currentAnimMove.shootPos.second)) {
                    continue;
                }
            }

            QPixmap *piecePixmap = nullptr;
            Chessboard::Cell cellType = m_chessboard->m_board[row][col];

            switch (cellType) {
            case Chessboard::Cell::Block:
                piecePixmap = &m_blockPiecePixmap;
                break;
            case Chessboard::Cell::White:
                piecePixmap = &m_whitePiecePixmap;
                break;
            case Chessboard::Cell::Black:
                piecePixmap = &m_blackPiecePixmap;
                break;
            case Chessboard::Cell::Empty:
                continue;
            }

            if (piecePixmap && !piecePixmap->isNull()) {
                // 缩放图片到合适大小
                // 选中的棋子会放大
                float scale = 0.75;
                int pieceSize = m_cellSize * scale;
                QPixmap scaledPixmap = piecePixmap->scaled(pieceSize, pieceSize,
                                Qt::KeepAspectRatio, Qt::SmoothTransformation);

                int verticalOffset = 0;
                // 游戏结束时播放庆祝动画
                if (m_gameOver) {
                    bool isWinner = false;
                    if (m_winner == Chessboard::Winner::White && cellType == Chessboard::Cell::White) {
                        isWinner = true;
                    } else if (m_winner == Chessboard::Winner::Black && cellType == Chessboard::Cell::Black) {
                        isWinner = true;
                    }

                    // 胜利方棋子跳动
                    if (isWinner) {
                        // 使用正弦函数计算跳动偏移
                        qreal progress = m_celebrationAnim->currentValue().toReal();
                        verticalOffset = -qAbs(qSin(progress * M_PI)) * m_cellSize * 0.3; // 向上跳
                    }
                } else if (row == selected.first && col == selected.second) {
                    scale = 0.9;
                }

                // 计算棋子坐标
                int left = m_marginX + m_cellSize * (col + (1 - scale) / 2);
                int top = m_marginY + m_cellSize * (row + (1 - scale) / 2) + verticalOffset;
                // 绘制棋子图片
                QRect pieceRect(left, top, pieceSize, pieceSize);
                painter.drawPixmap(pieceRect, scaledPixmap);

                // 失败方棋子叠加 block 图片
                if (m_gameOver && cellType != Chessboard::Cell::Block) {
                    bool shouldShowBlock = false;
                    if (m_winner == Chessboard::Winner::Tie) {
                        shouldShowBlock = true; // 平局时双方都显示
                    } else if (m_winner == Chessboard::Winner::White && cellType == Chessboard::Cell::Black) {
                        shouldShowBlock = true;
                    } else if (m_winner == Chessboard::Winner::Black && cellType == Chessboard::Cell::White) {
                        shouldShowBlock = true;
                    }

                    if (shouldShowBlock && !m_blockPiecePixmap.isNull()) {
                        QPixmap blockScaled = m_blockPiecePixmap.scaled(pieceSize, pieceSize,
                                                                        Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        painter.drawPixmap(pieceRect, blockScaled);
                    }
                }
            }
        }
    }

    // 绘制正在进行的动画层
    if (m_animType != None) {
        QPointF startPx = getCenterPos(m_animStartPos.x(), m_animStartPos.y());
        QPointF endPx = getCenterPos(m_animEndPos.x(), m_animEndPos.y());

        // 线性插值计算当前位置
        QPointF currentPx = startPx + (endPx - startPx) * m_animProgress;

        if (m_animType == Moving) {
            // 获取正在移动的棋子图片
            QPixmap *movingPiece = nullptr;
            Chessboard::Cell targetCell = m_chessboard->m_board[m_animEndPos.x()][m_animEndPos.y()];
            if (targetCell == Chessboard::Cell::White) movingPiece = &m_whitePiecePixmap;
            else if (targetCell == Chessboard::Cell::Black) movingPiece = &m_blackPiecePixmap;

            if (movingPiece) {
                float scale = 0.85; // 移动时保持选中状态的大小
                int pieceSize = m_cellSize * scale;
                QRectF rect(currentPx.x() - pieceSize/2, currentPx.y() - pieceSize/2, pieceSize, pieceSize);
                painter.drawPixmap(rect.toRect(), movingPiece->scaled(pieceSize, pieceSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
        else if (m_animType == Shooting) {
            if (!m_arrowPixmap.isNull()) {
                painter.save();

                // 计算旋转角度
                qreal angle = std::atan2(endPx.y() - startPx.y(), endPx.x() - startPx.x());
                qreal degrees = angle * 180.0 / M_PI;

                painter.translate(currentPx);
                painter.rotate(degrees - 90); // -90度因为原始图片向上

                // 绘制箭
                int arrowSize = m_cellSize;
                // 居中绘制：由于已经 translate 到中心，这里坐标是 (-w/2, -h/2)
                painter.drawPixmap(-arrowSize/2, -arrowSize/2, arrowSize, arrowSize, m_arrowPixmap);

                painter.restore();
            }
        }
    }
}

void ChessboardWidget::mousePressEvent(QMouseEvent *event)
{
    // 游戏结束或正在回放时禁止操作
    if (m_gameOver || m_chessboard->isReplaying()) {
        return;
    }

    // 判断是否为玩家的回合
    bool isWhitesTurn = (
            m_chessboard->m_turnState == Chessboard::TurnState::WhiteMove
            || m_chessboard->m_turnState == Chessboard::TurnState::WhiteShoot
        );
    if ((!m_chessboard->m_whiteIsPlayer && isWhitesTurn)
        || (!m_chessboard->m_blackIsPlayer && !isWhitesTurn)) {
        return; // 不是玩家的回合，忽略点击
    }

    if (event->button() == Qt::LeftButton) {
        // 计算光标在棋盘上的坐标
        QPoint boardPos = event->pos() - QPoint(m_marginX, m_marginY);
        // 定位光标指向的格子
        int row = boardPos.y() / m_cellSize;
        int col = boardPos.x() / m_cellSize;
        // 点到棋盘外，尝试取消选择
        if (boardPos.x() < 0 || boardPos.y() < 0 || row >= 8 || col >= 8) {
            m_chessboard->tryToClearSelected();
        }
        else {
            // 记录操作前的选中位置（即动画起始点）
            QPair<int, int> prevSelected = m_chessboard->m_selected;

            // 尝试行棋
            if (m_chessboard->moveSelectedTo(row, col)) {
                // 行棋成功，启动移动动画
                m_animType = Moving;
                m_animStartPos = QPoint(prevSelected.first, prevSelected.second);
                m_animEndPos = QPoint(row, col);
                m_animation->start();
            }
            // 尝试射箭
            else if (m_chessboard->shootAt(row, col)) {
                // 射箭成功，启动射箭动画
                m_animType = Shooting;
                // 箭从当前棋子位置发出
                m_animStartPos = QPoint(prevSelected.first, prevSelected.second);
                m_animEndPos = QPoint(row, col);
                m_animation->start();
            }
            else {
                // 尝试行棋失败，尝试选择棋子
                if (!m_chessboard->tryToSelect(row, col)) {
                    // 无效选择，清除之前选中的棋子
                    m_chessboard->tryToClearSelected();
                }
            }
        }
    }
    else if (event->button() == Qt::RightButton) {
        // 右键取消选择棋子
        m_chessboard->tryToClearSelected();
    }

    update(); // 触发重绘
}

QPointF ChessboardWidget::getCenterPos(int row, int col) const
{
    return QPointF(m_marginX + col * m_cellSize + m_cellSize / 2.0,
                   m_marginY + row * m_cellSize + m_cellSize / 2.0);
}

void ChessboardWidget::onAnimValueChanged(const QVariant &value)
{
    m_animProgress = value.toReal();
    update(); // 触发重绘
}

void ChessboardWidget::onAnimFinished()
{
    // 动画链处理
    if (m_animType == Moving) {
        // 移动动画结束，检查是否有射箭动作
        if (m_currentAnimMove.shootPos.first != -1 && m_currentAnimMove.shootPos.second != -1) {
            m_animType = Shooting;
            // 箭的起点是棋子移动后的位置 (即上一个动画的终点)
            m_animStartPos = m_animEndPos;
            // 箭的终点
            m_animEndPos = QPoint(m_currentAnimMove.shootPos.first, m_currentAnimMove.shootPos.second);

            m_animation->start(); // 再次启动动画
            return;
        }
    }

    // 全部动画结束，重置状态
    m_animType = None;
    m_animProgress = 0.0;
    m_currentAnimMove = Move();
    update();
}

void ChessboardWidget::onMoveMade(const Move &move, bool isWhite)
{
    // 如果当前已经是正在进行的动画(比如玩家鼠标操作触发)，则忽略
    if (m_animType != None) return;

    // 保存这次移动的信息
    m_currentAnimMove = move;

    // 启动第一阶段：棋子移动
    m_animType = Moving;
    m_animStartPos = QPoint(move.startPos.first, move.startPos.second);
    m_animEndPos = QPoint(move.targetPos.first, move.targetPos.second);

    // 启动动画
    m_animation->start();
}

void ChessboardWidget::onGameOver(Chessboard::Winner winner)
{
    m_gameOver = true;
    m_winner = winner;

    // 停止当前的移动/射箭动画
    if (m_animation->state() == QAbstractAnimation::Running) {
        m_animation->stop();
    }

    // 重置动画状态
    m_animType = Celebrating;
    m_animProgress = 0.0;
    m_currentAnimMove = Move();

    // 重置动画坐标,避免棋子被跳过绘制
    m_animStartPos = QPoint(-1, -1);
    m_animEndPos = QPoint(-1, -1);

    // 启动庆祝动画(仅胜利方跳动，非平局时)
    if (winner != Chessboard::Winner::Tie) {
        m_celebrationAnim->start();
    }

    update();
}

void ChessboardWidget::onReplayStarted()
{
    // 停止庆祝动画
    if (m_celebrationAnim->state() == QAbstractAnimation::Running) {
        m_celebrationAnim->stop();
    }

    // 重置游戏结束状态
    m_gameOver = false;
    m_animType = None;

    update();
}

void ChessboardWidget::onReplayStep(int currentStep, int totalSteps)
{
    // TODO: 在此处显示回放进度
}

void ChessboardWidget::onReplayFinished()
{
    // 回放结束，恢复游戏结束状态
    m_gameOver = true;

    // 重新启动庆祝动画
    if (m_winner != Chessboard::Winner::Tie) {
        m_celebrationAnim->start();
    }

    update();
}
