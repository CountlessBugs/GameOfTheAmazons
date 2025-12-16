#include "chessboard.h"

Chessboard::Chessboard(bool whiteIsPlayer, bool blackIsPlayer, bool autoCheckGameOver, QObject *parent)
    : QObject{parent}
    , m_whiteIsPlayer{whiteIsPlayer}
    , m_blackIsPlayer{blackIsPlayer}
    , m_autoCheckGameOver{autoCheckGameOver}
    , m_gameOver{false}
    , m_isReplaying{false}
    , m_replayStep{0}
{
    reset();

    // 初始化回放定时器
    m_replayTimer = new QTimer(this);
    m_replayTimer->setInterval(1000); // 每秒播放一步
    connect(m_replayTimer, &QTimer::timeout, this, &Chessboard::onReplayTimerTimeout);
}

bool Chessboard::makeMove(const Move &move)
{
    Chessboard::Cell cell = m_board[move.startPos.first][move.startPos.second];
    // 判断是否轮到该方的回合
    if ((cell == Chessboard::Cell::White
         && m_turnState == Chessboard::TurnState::WhiteMove)
        || (cell == Chessboard::Cell::Black
            && m_turnState == Chessboard::TurnState::BlackMove)) {
        // 尝试执行移动和射箭
        if (tryToSelect(move.startPos.first, move.startPos.second)
            && moveSelectedTo(move.targetPos.first, move.targetPos.second)
            && shootAt(move.shootPos.first, move.shootPos.second)) {

            tryToClearSelected();
            return true;
        }
        else return false;
    }
    return false;
}

bool Chessboard::tryToClearSelected()
{
    if (m_turnState == TurnState::WhiteMove
        || m_turnState == TurnState::BlackMove) {
        m_selected = {-1, -1};
        return true;
    }
    return false;
}

bool Chessboard::tryToSelect(int row, int col)
{
    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return false; // 越界
    }

    // 判断当前选择是否合法
    if ((m_turnState == TurnState::WhiteMove
        && m_board[row][col] == Cell::White)
        || (m_turnState == TurnState::BlackMove
        && m_board[row][col] == Cell::Black)) {
        m_selected = {row, col};
        return true;
    }
    return false;
}

bool Chessboard::moveSelectedTo(int row, int col)
{
    if (m_selected.first == -1) return false; // 未选中棋子

    // 判断是否轮到选中的一方行棋
    if (m_turnState != TurnState::WhiteMove
        && m_turnState != TurnState::BlackMove) {
        return false;
    } else if (m_turnState == TurnState::WhiteMove
               && m_board[m_selected.first][m_selected.second] != Cell::White) {
        return false;
    } else if (m_turnState == TurnState::BlackMove
               && m_board[m_selected.first][m_selected.second] != Cell::Black) {
        return false;
    }

    if (!pathValid(m_selected.first, m_selected.second, row, col)) {
        return false; // 路径无效
    };

    // 记录当前一步行棋的起始位置
    m_currentMove.startPos = {m_selected.first, m_selected.second};

    // 执行移动
    m_board[row][col] = m_board[m_selected.first][m_selected.second];
    m_board[m_selected.first][m_selected.second] = Cell::Empty;

    m_selected = {row, col}; // 刷新选中棋子的位置
    // 切换回合状态
    if (m_turnState == TurnState::WhiteMove) {
        m_turnState = TurnState::WhiteShoot;
    } else if (m_turnState == TurnState::BlackMove) {
        m_turnState = TurnState::BlackShoot;
    }

    // 记录当前一步行棋的目标位置
    m_currentMove.targetPos = {row, col};

    return true;
}

bool Chessboard::shootAt(int row, int col)
{
    if (m_selected.first == -1) return false; // 未选中棋子

    // 判断是否轮到选中的一方行棋
    if (m_turnState != TurnState::WhiteShoot
        && m_turnState != TurnState::BlackShoot) {
        return false;
    } else if (m_turnState == TurnState::WhiteShoot
               && m_board[m_selected.first][m_selected.second] != Cell::White) {
        return false;
    } else if (m_turnState == TurnState::BlackShoot
               && m_board[m_selected.first][m_selected.second] != Cell::Black) {
        return false;
    }

    if (!pathValid(m_selected.first, m_selected.second, row, col)) {
        return false; // 路径无效
    }

    m_board[row][col] = Cell::Block; // 执行射箭
    m_selected = {-1, -1}; // 清除选择

    // 切换回合状态
    if (m_turnState == TurnState::WhiteShoot) {
        m_turnState = TurnState::BlackMove;
    } else if (m_turnState == TurnState::BlackShoot) {
        m_turnState = TurnState::WhiteMove;
    }

    // 记录当前一步行棋的射箭位置
    m_currentMove.shootPos = {row, col};

    // 完成一次行棋后的处理
    // 记录历史
    m_history.push_back(m_currentMove);

    // 发出行棋信号
    emit moveMade(m_currentMove,
                  m_board[m_currentMove.targetPos.first][m_currentMove.targetPos.second] == Cell::White);

    // 检查游戏是否结束
    if (m_autoCheckGameOver) checkGameOver();

    return true;
}

bool Chessboard::checkGameOver()
{
    if (m_gameOver) return true;

    bool whiteCanMove = false;
    bool blackCanMove = false;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (m_board[r][c] == Cell::White) {
                if (canMove(r, c)) {
                    whiteCanMove = true;
                    if (blackCanMove) break;
                }
            } else if (m_board[r][c] == Cell::Black) {
                if (canMove(r, c)) {
                    blackCanMove = true;
                    if (whiteCanMove) break;
                }
            }
        }
        if (whiteCanMove && blackCanMove) break;
    }

    if (!whiteCanMove && !blackCanMove) onGameOver(Winner::Tie);
    else if (!whiteCanMove) onGameOver(Winner::Black);
    else if (!blackCanMove) onGameOver(Winner::White);

    return !(whiteCanMove && blackCanMove);
}

void Chessboard::startReplay()
{
    if (!m_gameOver) {
        return; // 只能在游戏结束后回放
    }

    if (m_isReplaying) {
        return; // 已经在回放中
    }

    // 拷贝历史记录
    m_replayHistory = m_history;

    // 重置棋盘到初始状态
    reset();

    // 设置回放状态
    m_isReplaying = true;
    m_replayStep = 0;
    m_gameOver = false; // 临时取消游戏结束状态，允许行棋

    emit replayStarted();
    emit boardLoaded(); // 通知界面重绘初始状态

    // 启动定时器
    m_replayTimer->start();
}

void Chessboard::stopReplay()
{
    if (!m_isReplaying) {
        return;
    }

    m_replayTimer->stop();
    m_isReplaying = false;
    m_gameOver = true; // 恢复游戏结束状态

    emit replayFinished();
}

void Chessboard::onReplayTimerTimeout()
{
    if (m_replayStep >= m_replayHistory.size()) {
        // 回放完成
        stopReplay();
        // 恢复完整的历史记录
        m_history = m_replayHistory;
        // 检查游戏结束状态
        checkGameOver();
        return;
    }

    // 执行当前步骤
    Move move = m_replayHistory[m_replayStep];

    // 临时禁用自动检查游戏结束
    bool autoCheck = m_autoCheckGameOver;
    m_autoCheckGameOver = false;

    // 执行走法
    makeMove(move);

    // 恢复自动检查设置
    m_autoCheckGameOver = autoCheck;

    // 更新回放步数
    m_replayStep++;

    emit replayStep(m_replayStep, m_replayHistory.size());
}

bool Chessboard::pathValid(int startRow, int startCol, int endRow, int endCol) const
{
    if (startRow < 0 || startRow >= 8 || startCol < 0 || startCol >= 8
        || endRow < 0 || endRow >= 8 || endCol < 0 || endCol >= 8) {
        return false; // 越界
    }

    if (m_board[endRow][endCol] != Cell::Empty) {
        return false; // 目标位置非空
    }

    // 判断是否为八向移动并判断路径是否存在障碍
    int diffRow = endRow - startRow;
    int diffCol = endCol - startCol;
    if (diffRow == 0 && diffCol != 0) {
        // 横向
        int step = (diffCol > 0) ? 1 : -1;
        for (int i = startCol + step; i != endCol; i += step) {
            if (m_board[startRow][i] != Cell::Empty) return false;
        }
    }
    else if (diffRow != 0 && diffCol == 0) {
        // 竖向
        int step = (diffRow > 0) ? 1 : -1;
        for (int i = startRow + step; i != endRow; i += step) {
            if (m_board[i][startCol] != Cell::Empty) return false;
        }
    }
    else if ((diffRow == diffCol && diffRow != 0)
             || (diffRow == -diffCol && diffRow != 0)) {
        // 斜向
        int stepRow = (diffRow > 0) ? 1 : -1;
        int stepCol = (diffCol > 0) ? 1 : -1;
        int r =startRow + stepRow;
        int c = startCol + stepCol;
        while (r != endRow || c != endCol) {
            if (m_board[r][c] != Cell::Empty) return false;
            r += stepRow;
            c += stepCol;
        }
    }
    else return false;

    return true;
}

void Chessboard::onGameOver(Winner winner)
{
    m_gameOver = true;
    emit gameOver(winner);
}

int Chessboard::getTerritoryArea(int row, int col, bool isWhite, Board *board) const
{
    // 拷贝棋盘，用于洪水填充
    Board boardCopy;
    if (!board) {
        boardCopy = m_board;
        board = &boardCopy;
    }

    if ((*board)[row][col] == (isWhite ? Cell::Black : Cell::White)) {
        // 遇到敌方棋子，说明不在官子区域
        return -1;
    }
    else if ((*board)[row][col] == Cell::Block) {
        // 遇到障碍物，停止探索该方向
        return 0;
    }

    int area = ((*board)[row][col] == Cell::Empty ? 1 : 0); // 当前空白格子计入面积

    // 当前为空地或己方棋子，标记为已访问
    (*board)[row][col] = Cell::Block;

    // 向八个方向探索
    if (row > 0) {
        int delta = getTerritoryArea(row - 1, col, isWhite, board);
        if (delta == -1) return -1;
        area += delta;
    }
    if (row < 7) {
        int delta = getTerritoryArea(row + 1, col, isWhite, board);
        if (delta == -1) return -1;
        area += delta;
    }
    if (col > 0) {
        int delta = getTerritoryArea(row, col - 1, isWhite, board);
        if (delta == -1) return -1;
        area += delta;
    }
    if (col < 7) {
        int delta = getTerritoryArea(row, col + 1, isWhite, board);
        if (delta == -1) return -1;
        area += delta;
    }
    if (row > 0 && col > 0) {
        int delta = getTerritoryArea(row - 1, col - 1, isWhite, board);
        if (delta == -1) return -1;
        area += delta;
    }
    if (row > 0 && col < 7) {
        int delta = getTerritoryArea(row - 1, col + 1, isWhite, board);
        if (delta == -1) return -1;
        area += delta;
    }
    if (row < 7 && col > 0) {
        int delta = getTerritoryArea(row + 1, col - 1, isWhite, board);
        if (delta == -1) return -1;
        area += delta;
    }
    if (row < 7 && col < 7) {
        int delta = getTerritoryArea(row + 1, col + 1, isWhite, board);
        if (delta == -1) return -1;
        area += delta;
    }
    return area;
}

MoveRange Chessboard::getMoveRange(int row, int col) const
{
    MoveRange range;

    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return range; // 越界
    }

    // 探索八个方向
    // 向上
    while (row - range.up - 1 >= 0
           && m_board[row - range.up - 1][col] == Cell::Empty) {
        range.up++;
    }
    // 向下
    while (row + range.down + 1 < 8
           && m_board[row + range.down + 1][col] == Cell::Empty) {
        range.down++;
    }
    // 向左
    while (col - range.left - 1 >= 0
            && m_board[row][col - range.left - 1] == Cell::Empty) {
        range.left++;
    }
    // 向右
    while (col + range.right + 1 < 8
            && m_board[row][col + range.right + 1] == Cell::Empty) {
        range.right++;
    }
    // 向左上
    while (row - range.upLeft - 1 >= 0
            && col - range.upLeft - 1 >= 0
            && m_board[row - range.upLeft - 1][col - range.upLeft - 1] == Cell::Empty) {
        range.upLeft++;
    }
    // 向右上
    while (row - range.upRight - 1 >= 0
            && col + range.upRight + 1 < 8
            && m_board[row - range.upRight - 1][col + range.upRight + 1] == Cell::Empty) {
        range.upRight++;
    }
    // 向左下
    while (row + range.downLeft + 1 < 8
            && col - range.downLeft - 1 >= 0
            && m_board[row + range.downLeft + 1][col - range.downLeft - 1] == Cell::Empty) {
        range.downLeft++;
    }
    // 向右下
    while (row + range.downRight + 1 < 8
            && col + range.downRight + 1 < 8
            && m_board[row + range.downRight + 1][col + range.downRight + 1] == Cell::Empty) {
        range.downRight++;
    }

    range.territoryArea = getTerritoryArea(row, col, m_board[row][col] == Cell::White);

    return range;
}

MoveRange Chessboard::getMoveRangeIgnoring(int row, int col, int ignoreRow, int ignoreCol)
{
    // 临时将棋盘上的该位置替换成空
    Chessboard::Cell originalCell = m_board[ignoreRow][ignoreCol];
    m_board[ignoreRow][ignoreCol] = Cell::Empty;
    // 获取该位置的活动范围
    MoveRange result = getMoveRange(row, col);
    // 恢复该位置的棋子
    m_board[ignoreRow][ignoreCol] = originalCell;
    return result;
}

bool Chessboard::canMove(int row, int col) const
{
    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return false; // 越界
    }
    // 探索八个方向
    if (row > 0 && m_board[row - 1][col] == Cell::Empty) return true; // 向上
    if (row < 7 && m_board[row + 1][col] == Cell::Empty) return true; // 向下
    if (col > 0 && m_board[row][col - 1] == Cell::Empty) return true; // 向左
    if (col < 7 && m_board[row][col + 1] == Cell::Empty) return true; // 向右
    if (row > 0 && col > 0 && m_board[row - 1][col - 1] == Cell::Empty) return true; // 向左上
    if (row > 0 && col < 7 && m_board[row - 1][col + 1] == Cell::Empty) return true; // 向右上
    if (row < 7 && col > 0 && m_board[row + 1][col - 1] == Cell::Empty) return true; // 向左下
    if (row < 7 && col < 7 && m_board[row + 1][col + 1] == Cell::Empty) return true; // 向右下

    return false;
}

void Chessboard::reset() {
    m_board = std::array<std::array<Cell, 8>, 8>();
    m_board[0][2] = Cell::Black;
    m_board[0][5] = Cell::Black;
    m_board[2][0] = Cell::Black;
    m_board[2][7] = Cell::Black;
    m_board[7][2] = Cell::White;
    m_board[7][5] = Cell::White;
    m_board[5][0] = Cell::White;
    m_board[5][7] = Cell::White;

    m_selected = {-1, -1};
    m_turnState = TurnState::WhiteMove;
}
