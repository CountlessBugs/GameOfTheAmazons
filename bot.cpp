#include "bot.h"
#include <QTimer>

// 预设权重配置
const Bot::Weights Bot::EZ_WEIGHTS{ 1.90, 0.147, 0.604, 0.108, 2.00, 0.222, 0.61 };
const Bot::Weights Bot::MD_WEIGHTS{ 1.75, 0.0357, 0.939, 0.184, 2.00, 0.623, 0.80 };
const Bot::Weights Bot::HD_WEIGHTS{ 1.00, 0.500, 0.600, 0.400, 2.00, 0.500, 0.300 };

Bot::Bot(Chessboard *chessboard, bool isWhite, Weights weights, QObject *parent)
    : QObject{parent}
    , m_chessboard{chessboard}
    , m_isWhite{isWhite}
    , m_gameOver{false}
    , m_endgame{false}
    , m_weights{weights}
{
    m_sandbox = new Chessboard(true, true, false, this); // 创建沙盒棋盘

    // 连接对手走棋信号
    connect(m_chessboard, &Chessboard::moveMade, this, [this](const Move &move, bool isWhite) {
        if (isWhite != m_isWhite) {
            // 对手走完后，延迟 1000ms AI再行动
            QTimer::singleShot(1000, this, [this]() {
                makeNextMove();
            });
        }
    });

    // 游戏结束时重置AI状态
    connect(m_chessboard, &Chessboard::gameOver, this, [this](Chessboard::Winner winner) {
        Q_UNUSED(winner);
        m_gameOver = true;
        reset();
    });

    // 如果是AI先手，立即行动
    if ((m_isWhite && m_chessboard->m_turnState == Chessboard::TurnState::WhiteMove)
        || (!m_isWhite && m_chessboard->m_turnState == Chessboard::TurnState::BlackMove)) {
        QTimer::singleShot(200, this, [this]() {
            makeNextMove();
        });
    }
}

bool Bot::makeNextMove()
{
    if (m_gameOver) return false;

    // 官子阶段调用官子走法
    if (m_endgame) return makeMoveInEndgame();
    else if (getAllMoves().isEndgame()) {
        m_endgame = true;
        return makeMoveInEndgame();
    }

    resetSandbox();
    // 动态调整搜索深度
    int depth = 0;
    if (m_chessboard->m_history.size() < 6) {
        depth = 1;
    } else if (m_chessboard->m_history.size() < 24) {
        depth = 2;
    } else if (m_chessboard->m_history.size() < 48) {
        depth = 3;
    } else {
        depth = 4;
    }
    Move move = getBestMove(depth);
    return makeMove(move);
}

void Bot::reset()
{
    m_endgame = false;
    m_endgameMoves.clear();
    m_endgameVisited.clear();
    m_sandbox->reset();
}

Bot::AllMoves Bot::getAllMoves(bool inSandbox) const
{
    return getAllMovesForSide(m_isWhite, inSandbox);
}

Bot::AllMoves Bot::getAllMovesForSide(bool isWhite, bool inSandbox) const
{
    AllMoves allMoves;
    int index = 0; // 棋子索引

    Chessboard *chessboard = inSandbox ? m_sandbox : m_chessboard;

    // 获取所有棋子位置与可行动范围
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if ((isWhite && chessboard->m_board[r][c] == Chessboard::Cell::White)
                || (!isWhite && chessboard->m_board[r][c] == Chessboard::Cell::Black))
            {
                allMoves.positions[index] = {r, c};
                allMoves.moves[index] = chessboard->getMoveRange(r, c);

                // 非官子阶段下跳过处于官子状态的棋子
                if (!m_endgame && allMoves.moves[index].inClosedRegion()) {
                    index++;
                    continue;
                }

                allMoves.moveOpts += allMoves.moves[index].getTotalMoves();

                // 获取当前每种移动方式对应的射箭方式总数
                for (int dir = 0; dir < 8; dir++) {
                    int moveCount = 0;
                    switch (dir) {
                    case 0: moveCount = allMoves.moves[index].up; break;
                    case 1: moveCount = allMoves.moves[index].upRight; break;
                    case 2: moveCount = allMoves.moves[index].right; break;
                    case 3: moveCount = allMoves.moves[index].downRight; break;
                    case 4: moveCount = allMoves.moves[index].down; break;
                    case 5: moveCount = allMoves.moves[index].downLeft; break;
                    case 6: moveCount = allMoves.moves[index].left; break;
                    case 7: moveCount = allMoves.moves[index].upLeft; break;
                    }

                    for (int step = 1; step <= moveCount; step++) {
                        // 模拟移动后获取射箭范围
                        int newRow = allMoves.positions[index].first;
                        int newCol = allMoves.positions[index].second;
                        switch (dir) {
                        case 0: newRow -= step; break;
                        case 1: newRow -= step; newCol += step; break;
                        case 2: newCol += step; break;
                        case 3: newRow += step; newCol += step; break;
                        case 4: newRow += step; break;
                        case 5: newRow += step; newCol -= step; break;
                        case 6: newCol -= step; break;
                        case 7: newRow -= step; newCol -= step; break;
                        }

                        // 使用正确的棋盘对象
                        MoveRange shootRange = chessboard->getMoveRangeIgnoring(
                            newRow, newCol,
                            allMoves.positions[index].first,
                            allMoves.positions[index].second
                            );

                        int curShootOpts = shootRange.getTotalMoves();
                        allMoves.shootOpts.append(curShootOpts);
                        allMoves.actionCount += curShootOpts;
                    }
                }
                index++;
            }
        }
    }

    return allMoves;
}

bool Bot::makeMove(const Move &move)
{
    Chessboard::Cell cell = m_chessboard->m_board[move.startPos.first][move.startPos.second];
    // 判断移动的棋子是否属于AI
    if ((m_isWhite && cell == Chessboard::Cell::White)
        || (!m_isWhite && cell == Chessboard::Cell::Black)) {
        return m_chessboard->makeMove(move);
    }
    return false;
}

QVector<Move> Bot::getEndgameMoves(QPair<int, int> piecePos)
{
    // 初始化：寻找第一个可移动的棋子
    if (piecePos.first == -1) {
        Bot::AllMoves allMoves = getAllMoves();
        m_endgameMoves.clear();
        m_endgameVisited.clear();
        resetSandbox();

        // 获取第一个可行动的棋子位置
        int index = 0;
        for (; index < 4; index++) {
            if (allMoves.moves[index].getTotalMoves() > 0) {
                piecePos = allMoves.positions[index];
                break;
            }
        }

        // 空格过多时直接返回第一种走法
        MoveRange curMoveRange = allMoves.moves[index];
        if (curMoveRange.territoryArea > 12) {
            QVector<Move> allLegalMoves = generateLegalMoves(m_isWhite, true);
            // 筛选出该棋子的走法并返回第一个
            for (const Move &move : std::as_const(allLegalMoves)) {
                if (move.startPos == piecePos) {
                    return {move};
                }
            }
            return {};
        }
        return getEndgameMoves(piecePos);
    }

    // 防止重复访问相同局面
    Chessboard::Board board = m_sandbox->m_board;
    if (m_endgameVisited.contains(board)) {
        return {};
    }
    m_endgameVisited.insert(board);

    // 获取当前棋子的移动范围
    MoveRange range = m_sandbox->getMoveRange(piecePos.first, piecePos.second);

    // 使用generateLegalMoves替代手动遍历算法
    QVector<Move> bestMoves;

    // 获取当前玩家在沙盘中的所有合法走法
    QVector<Move> allLegalMoves = generateLegalMoves(m_isWhite, true);

    // 筛选出指定棋子的走法（保持与原逻辑一致）
    QVector<Move> pieceMoves;
    for (const Move &move : std::as_const(allLegalMoves)) {
        if (move.startPos == piecePos) {
            pieceMoves.append(move);
        }
    }

    // 遍历该棋子的所有走法进行回溯搜索
    for (const Move &move : pieceMoves) {
        if (makeMoveInSandbox(move, true, true)) {
            // 递归搜索后续走法序列
            QVector<Move> followingMoves = getEndgameMoves(move.targetPos);

            // 选择最长的走法序列
            if (followingMoves.size() + 1 > bestMoves.size()) {
                bestMoves = followingMoves;
                bestMoves.push_front(move);
            }

            // 撤销走法
            reverseMoveInSandbox(move);
        }

        // 提前终止条件：已达到最大可能步数
        if (range.territoryArea == bestMoves.size()) {
            break;
        }
    }

    return bestMoves;
}

bool Bot::makeMoveInEndgame()
{
    if (m_endgameMoves.size() == 0) {
        m_endgameMoves = getEndgameMoves();
    }

    if (m_endgameMoves.size() > 0) {
        Move move = m_endgameMoves.takeFirst(); // 提取并移除首个元素
        bool succeed = m_chessboard->makeMove(move);
        return succeed;
    }
    return false;
}

bool Bot::makeMoveInSandbox(const Move &move, bool skipOpponentTurn, bool autoSetTurnState)
{
    if (autoSetTurnState) {
        m_sandbox->m_turnState = (m_sandbox->m_board[move.startPos.first][move.startPos.second] == Chessboard::Cell::White
                                ? Chessboard::TurnState::WhiteMove
                                : Chessboard::TurnState::BlackMove);
    }

    if (m_sandbox->makeMove(move)) {
        if (skipOpponentTurn) {
            // 跳过对手回合
            m_sandbox->m_turnState = (m_isWhite ? Chessboard::TurnState::WhiteMove
                                              : Chessboard::TurnState::BlackMove);
        }
        return true;
    }
    else return false;
}

bool Bot::reverseMoveInSandbox(const Move &move)
{
    Chessboard::Cell cell = m_sandbox->m_board[move.targetPos.first][move.targetPos.second];
    if (cell == Chessboard::Cell::Empty || cell == Chessboard::Cell::Block) {
        return false;
    }
    Chessboard::Board board = m_sandbox->m_board;
    // 撤销射箭
    board[move.shootPos.first][move.shootPos.second] = Chessboard::Cell::Empty;
    // 撤销移动
    board[move.startPos.first][move.startPos.second] = cell;
    board[move.targetPos.first][move.targetPos.second] = Chessboard::Cell::Empty;

    m_sandbox->m_board = board;
    return true;
}

void Bot::resetSandbox()
{
    m_sandbox->m_board = m_chessboard->m_board;
    m_sandbox->m_turnState = m_chessboard->m_turnState;
}

double Bot::evalSandbox()
{
    // 获取双方的走法信息
    AllMoves myMoves = getAllMoves(true);
    AllMoves oppMoves = getAllMovesForSide(!m_isWhite, true);

    double score = 0.0;

    // 1. 移动性评分
    int myTotalMoves = myMoves.moveOpts;
    int oppTotalMoves = oppMoves.moveOpts;

    // 幂函数调整
    double mobilityDiffVal = std::pow(std::abs((double)(myTotalMoves - oppTotalMoves)), m_weights.mobilityExponent);
    mobilityDiffVal *= (myTotalMoves - oppTotalMoves >= 0 ? 1 : -1);
    double mobilityScore = mobilityDiffVal * m_weights.mobilityWeight;
    // 标准化
    mobilityScore /= 4.0 * 27.0;
    score += mobilityScore;

    // 2. 射箭灵活性评分
    int myShootOpts = 0;
    for (int opts : std::as_const(myMoves.shootOpts)) {
        myShootOpts += opts;
    }
    int oppShootOpts = 0;
    for (int opts : std::as_const(oppMoves.shootOpts)) {
        oppShootOpts += opts;
    }

    // 幂函数调整
    double shootDiffVal = std::pow(std::abs((double)(myShootOpts - oppShootOpts)), m_weights.shootExponent);
    shootDiffVal *= (myShootOpts - oppShootOpts >= 0 ? 1 : -1);
    double shootScore = shootDiffVal * m_weights.shootFlexibilityWeight;
    // 标准化
    shootScore /= 4.0 * 27.0 * 27.0;
    score += shootScore;

    // 3. 官子阶段特殊处理
    if (myMoves.isEndgame() || oppMoves.isEndgame()) {
        // 计算领地大小
        int myTerritory = 0;
        int oppTerritory = 0;

        for (int i = 0; i < 4; i++) {
            if (myMoves.moves[i].inClosedRegion()) {
                myTerritory += myMoves.moves[i].territoryArea;
            }
            if (oppMoves.moves[i].inClosedRegion()) {
                oppTerritory += oppMoves.moves[i].territoryArea;
            }
        }

        double territoryScore = (myTerritory - oppTerritory) * m_weights.territoryWeight;
        // 标准化
        territoryScore /= 54.0;
        score += territoryScore;
    } else {
        // 4. 非官子阶段：中心控制评分
        double centerScore = 0.0;
        for (int i = 0; i < 4; i++) {
            QPair<int, int> myPos = myMoves.positions[i];
            QPair<int, int> oppPos = oppMoves.positions[i];

            // 计算曼哈顿距离(中心为3.5, 3.5)
            double myDistToCenter = std::abs(myPos.first - 3.5)
                                    + std::abs(myPos.second - 3.5);
            double oppDistToCenter = std::abs(oppPos.first - 3.5)
                                     + std::abs(oppPos.second - 3.5);

            // 距离中心越近越好
            centerScore += (oppDistToCenter - myDistToCenter);
        }
        // 标准化
        centerScore /= 28.0;
        score += centerScore * m_weights.centerControlWeight;

        // 5. 分散性评分
        double myDistance = 0.0;
        double oppDistance = 0.0;

        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                // 使用曼哈顿距离
                int myDist = std::abs(myMoves.positions[i].first - myMoves.positions[j].first)
                             + std::abs(myMoves.positions[i].second - myMoves.positions[j].second);
                int oppDist = std::abs(oppMoves.positions[i].first - oppMoves.positions[j].first)
                              + std::abs(oppMoves.positions[i].second - oppMoves.positions[j].second);

                myDistance += myDist;
                oppDistance += oppDist;
            }
        }

        // 距离越大越好
        double dispersionScore = (myDistance - oppDistance) * m_weights.dispersionWeight;
        // 标准化
        dispersionScore /= 56.0;
        score += dispersionScore;
    }

    // 特殊判断：如果对手无法移动，给予巨大奖励
    if (oppTotalMoves == 0) {
        score += 1e9;
    }
    // 如果自己无法移动，给予巨大惩罚
    if (myTotalMoves == 0) {
        score -= 1e9;
    }

    return score;
}

Move Bot::getBestMove(int depth)
{
    QVector<Move> moves = generateLegalMoves(m_isWhite, true);
    if (moves.isEmpty()) return Move();

    Move bestMove;
    double maxEval = -1e11;
    double alpha = -1e11;
    double beta = 1e11;

    // 在根节点进行搜索
    for (const Move &move : std::as_const(moves)) {
        // 在沙盘执行一步
        if (makeMoveInSandbox(move, true, true)) {
            // 递归调用 Alpha-Beta，当前是 Max 层，下一层是 Min 层
            double eval = alphaBeta(depth - 1, alpha, beta, false);

            // 撤销一步
            reverseMoveInSandbox(move);

            if (eval > maxEval) {
                maxEval = eval;
                bestMove = move;
            }

            // 更新 Alpha
            if (eval > alpha) {
                alpha = eval;
            }
            // 根节点不需要 Beta 剪枝，因为我们必须找出一个动作
        }
    }

    // 如果没有找到有效走法，返回空
    if (bestMove.startPos.first == -1 && !moves.isEmpty()) {
        return moves[0];
    }

    return bestMove;
}

double Bot::alphaBeta(int depth, double alpha, double beta, bool maximizingPlayer)
{
    // 终止条件：达到深度或游戏结束
    if (depth == 0) {
        return evalSandbox();
    }

    // 确定当前模拟的是谁的走法
    bool currentSideIsWhite = maximizingPlayer ? m_isWhite : !m_isWhite;

    // 生成当前回合方所有可能的走法
    QVector<Move> moves = generateLegalMoves(currentSideIsWhite, true);

    // 如果无路可走
    if (moves.isEmpty()) {
        // 如果是 maximizingPlayer 无路可走，说明AI输了，返回极小值
        // 如果是 minimizingPlayer 无路可走，说明对手输了，返回极大值
        return maximizingPlayer ? -1e10 : 1e10;
    }

    if (maximizingPlayer) {
        double maxEval = -1e11;
        for (const Move &move : std::as_const(moves)) {
            if (makeMoveInSandbox(move, true, true)) {
                double eval = alphaBeta(depth - 1, alpha, beta, false);
                reverseMoveInSandbox(move);

                if (eval > maxEval) maxEval = eval;
                if (eval > alpha) alpha = eval;

                // Beta 剪枝：对手已经找到了一个比当前路径更坏(对AI来说)的选项
                // 所以对手绝对不会让局面到达现在的 alpha 状态
                if (beta <= alpha) {
                    break;
                }
            }
        }
        return maxEval;
    } else {
        double minEval = 1e11;
        for (const Move &move : std::as_const(moves)) {
            if (makeMoveInSandbox(move, true, true)) {
                double eval = alphaBeta(depth - 1, alpha, beta, true);
                reverseMoveInSandbox(move);

                if (eval < minEval) minEval = eval;
                if (eval < beta) beta = eval;

                // Alpha 剪枝：AI 已经找到了一个比当前路径更好(对AI来说)的选项
                // 所以 AI 绝不会选择进入这个分支
                if (beta <= alpha) {
                    break;
                }
            }
        }
        return minEval;
    }
}

QVector<Move> Bot::generateLegalMoves(bool isWhite, bool inSandbox) const
{
    QVector<Move> moveList;
    // 预分配内存以减少扩容开销，估算值
    moveList.reserve(512);

    Chessboard *chessboard = inSandbox ? m_sandbox : m_chessboard;
    const auto &board = chessboard->m_board;

    // 找到所有己方棋子
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if ((isWhite && board[r][c] == Chessboard::Cell::White) ||
                (!isWhite && board[r][c] == Chessboard::Cell::Black)) {

                // 获取该棋子的移动范围
                MoveRange range = chessboard->getMoveRange(r, c);

                // 官子阶段检查
                if (!m_endgame && range.inClosedRegion()) {
                    continue; // 非官子阶段跳过封闭区域
                }

                if (!range.canMove()) continue;

                // 遍历所有移动方向和步数
                for (int dir = 0; dir < 8; dir++) {
                    int steps = 0;
                    switch(dir) {
                    case 0: steps = range.up; break;
                    case 1: steps = range.upRight; break;
                    case 2: steps = range.right; break;
                    case 3: steps = range.downRight; break;
                    case 4: steps = range.down; break;
                    case 5: steps = range.downLeft; break;
                    case 6: steps = range.left; break;
                    case 7: steps = range.upLeft; break;
                    }

                    for (int s = 1; s <= steps; ++s) {
                        int targetR = r, targetC = c;
                        // 计算目标坐标
                        switch(dir) {
                        case 0: targetR -= s; break;
                        case 1: targetR -= s; targetC += s; break;
                        case 2: targetC += s; break;
                        case 3: targetR += s; targetC += s; break;
                        case 4: targetR += s; break;
                        case 5: targetR += s; targetC -= s; break;
                        case 6: targetC -= s; break;
                        case 7: targetR -= s; targetC -= s; break;
                        }

                        // 在目标位置计算射箭范围
                        // 此时棋子假定已移动到 targetR, targetC，原位置 r, c 视为空
                        MoveRange shootRange = chessboard->getMoveRangeIgnoring(targetR, targetC, r, c);

                        // 遍历所有射箭方向
                        for (int shootDir = 0; shootDir < 8; shootDir++) {
                            int shootSteps = 0;
                            switch(shootDir) {
                            case 0: shootSteps = shootRange.up; break;
                            case 1: shootSteps = shootRange.upRight; break;
                            case 2: shootSteps = shootRange.right; break;
                            case 3: shootSteps = shootRange.downRight; break;
                            case 4: shootSteps = shootRange.down; break;
                            case 5: shootSteps = shootRange.downLeft; break;
                            case 6: shootSteps = shootRange.left; break;
                            case 7: shootSteps = shootRange.upLeft; break;
                            }

                            for (int ss = 1; ss <= shootSteps; ++ss) {
                                int shootR = targetR, shootC = targetC;
                                switch(shootDir) {
                                case 0: shootR -= ss; break;
                                case 1: shootR -= ss; shootC += ss; break;
                                case 2: shootC += ss; break;
                                case 3: shootR += ss; shootC += ss; break;
                                case 4: shootR += ss; break;
                                case 5: shootR += ss; shootC -= ss; break;
                                case 6: shootC -= ss; break;
                                case 7: shootR -= ss; shootC -= ss; break;
                                }

                                // 构造 Move 对象
                                Move move;
                                move.startPos = {r, c};
                                move.targetPos = {targetR, targetC};
                                move.shootPos = {shootR, shootC};
                                moveList.append(move);
                            }
                        }
                    }
                }
            }
        }
    }
    return moveList;
}
