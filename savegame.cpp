#include "savegame.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SaveGame::SaveGame(Chessboard *chessboard, Bot *whiteBot, Bot *blackBot, QObject *parent)
    : QObject{parent}
    , m_chessboard{chessboard}
    , m_whiteBot{whiteBot}
    , m_blackBot{blackBot}
{}

QJsonArray SaveGame::boardToJsonArray(const Chessboard::Board &board)
{
    QJsonArray boardArray;

    // 保存棋盘状态至数组
    for (const auto &row : board) {
        QJsonArray rowArray;
        for (const auto cell : row) {
            rowArray.append(static_cast<int>(cell));
        }
        boardArray.append(rowArray);
    }

    return boardArray;
}

Chessboard::Board SaveGame::jsonArrayToBoard(const QJsonArray &boardArray)
{
    Chessboard::Board board;

    if (boardArray.size() != 8) {
        return board; // 返回空棋盘
    }

    for (int r = 0; r < 8; r++) {
        QJsonArray rowArray = boardArray[r].toArray();
        if (rowArray.size() != 8) {
            return board; // 返回空棋盘
        }
        for (int c = 0; c < 8; c++) {
            board[r][c] = static_cast<Chessboard::Cell>(rowArray[c].toInt());
        }
    }

    return board;
}

bool SaveGame::saveGameAsJson()
{
    // 获取保存目录
    QDir dir;
    if (!dir.exists("./saves")) {
        dir.mkpath("./saves");
    }

    // 生成时间戳
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
    // 生成文件路径
    QString filePath = QString("%1/%2.json").arg("./saves", timestamp);
    QFile file(filePath);

    // 尝试打开文件
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    // 构建保存对象
    QJsonObject saveObj;

    // 如果正在回放，保存回放历史记录中的完整状态
    QVector<Move> historyToSave;
    if (m_chessboard->isReplaying()) {
        // 获取完整的回放历史记录
        historyToSave = m_chessboard->getReplayHistory();

        // 临时重建最终棋盘状态
        Chessboard tempBoard(m_chessboard->m_whiteIsPlayer,
                             m_chessboard->m_blackIsPlayer,
                             false); // 禁用自动检查游戏结束

        // 执行所有历史走法
        for (const Move &move : std::as_const(historyToSave)) {
            tempBoard.makeMove(move);
        }

        // 保存最终状态
        saveObj["board"] = boardToJsonArray(tempBoard.m_board);
        saveObj["turnState"] = static_cast<int>(tempBoard.m_turnState);
        saveObj["selected"] = QJsonArray{
            tempBoard.m_selected.first,
            tempBoard.m_selected.second
        };
    } else {
        // 正常保存当前状态
        historyToSave = m_chessboard->m_history;
        saveObj["board"] = boardToJsonArray(m_chessboard->m_board);
        saveObj["turnState"] = static_cast<int>(m_chessboard->m_turnState);
        saveObj["selected"] = QJsonArray{
            m_chessboard->m_selected.first,
            m_chessboard->m_selected.second
        };
    }

    // 保存棋手身份
    saveObj["whiteIsPlayer"] = m_chessboard->m_whiteIsPlayer;
    saveObj["blackIsPlayer"] = m_chessboard->m_blackIsPlayer;

    // 保存 Bot 权重
    if (m_whiteBot) {
        QJsonObject whiteBotObj;
        whiteBotObj["weights"] = m_whiteBot->getWeights().toJson();
        saveObj["whiteBot"] = whiteBotObj;
    }
    if (m_blackBot) {
        QJsonObject blackBotObj;
        blackBotObj["weights"] = m_blackBot->getWeights().toJson();
        saveObj["blackBot"] = blackBotObj;
    }

    // 保存历史记录
    QJsonArray historyArray;
    for (const Move &move : std::as_const(historyToSave)) {
        QJsonObject moveObj;
        moveObj["startRow"] = move.startPos.first;
        moveObj["startCol"] = move.startPos.second;
        moveObj["targetRow"] = move.targetPos.first;
        moveObj["targetCol"] = move.targetPos.second;
        moveObj["shootRow"] = move.shootPos.first;
        moveObj["shootCol"] = move.shootPos.second;
        historyArray.append(moveObj);
    }
    saveObj["history"] = historyArray;

    // 转换为JSON文档
    QJsonDocument saveDoc(saveObj);

    // 写入文件
    file.write(saveDoc.toJson());
    file.close();
    return true;
}

bool SaveGame::loadSave(QString filePath)
{
    QFile file(filePath);

    // 尝试打开文件
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray saveData = file.readAll();
    file.close();

    // 读取Json内容
    QJsonDocument saveDoc = QJsonDocument::fromJson(saveData);
    if (saveDoc.isNull() || !saveDoc.isObject()) {
        return false;
    }

    QJsonObject saveObj = saveDoc.object();
    if (!saveObj.contains("board") || !saveObj["board"].isArray()) {
        return false;
    }

    QJsonArray boardArray = saveObj["board"].toArray();
    if (boardArray.size() != 8) {
        return false;
    }

    // 将boardArray转换回棋盘状态
    Chessboard::Board board = jsonArrayToBoard(boardArray);
    // 设置棋盘状态
    m_chessboard->m_board = board;

    // 设置回合状态
    if (saveObj.contains("turnState") && saveObj["turnState"].isDouble()) {
        int turnStateInt = saveObj["turnState"].toInt();
        if (turnStateInt >= 0 && turnStateInt <= 3) {
            m_chessboard->m_turnState = static_cast<Chessboard::TurnState>(turnStateInt);
        }
    }

    // 设置当前选中的棋子
    if (saveObj.contains("selected") && saveObj["selected"].isArray()) {
        QJsonArray selectedArray = saveObj["selected"].toArray();
        if (selectedArray.size() == 2) {
            int row = selectedArray[0].toInt();
            int col = selectedArray[1].toInt();
            if (row >= 0 && row < 8 && col >= 0 && col < 8) {
                m_chessboard->m_selected = QPair<int, int>(row, col);
            }
        }
    }

    // 设置棋手身份
    if (saveObj.contains("whiteIsPlayer") && saveObj["whiteIsPlayer"].isBool()) {
        m_chessboard->m_whiteIsPlayer = saveObj["whiteIsPlayer"].toBool();
    }
    if (saveObj.contains("blackIsPlayer") && saveObj["blackIsPlayer"].isBool()) {
        m_chessboard->m_blackIsPlayer = saveObj["blackIsPlayer"].toBool();
    }

    // 设置 Bot 权重
    Bot::Weights whiteWeights;
    if (saveObj.contains("whiteBot") && saveObj["whiteBot"].isObject()) {
        QJsonObject whiteBotObj = saveObj["whiteBot"].toObject();
        if (whiteBotObj.contains("weights") && whiteBotObj["weights"].isObject()) {
            whiteWeights.fromJson(whiteBotObj["weights"].toObject());
        }
    }
    Bot::Weights blackWeights;
    if (saveObj.contains("blackBot") && saveObj["blackBot"].isObject()) {
        QJsonObject blackBotObj = saveObj["blackBot"].toObject();
        if (blackBotObj.contains("weights") && blackBotObj["weights"].isObject()) {
            blackWeights.fromJson(blackBotObj["weights"].toObject());
        }
    }

    // 设置历史记录
    if (saveObj.contains("history") && saveObj["history"].isArray()) {
        QJsonArray historyArray = saveObj["history"].toArray();
        QVector<Move> history;
        for (const QJsonValue &value : std::as_const(historyArray)) {
            if (value.isObject()) {
                QJsonObject moveObj = value.toObject();
                Move move;
                move.startPos = {
                    moveObj["startRow"].toInt(),
                    moveObj["startCol"].toInt()
                };
                move.targetPos = {
                    moveObj["targetRow"].toInt(),
                    moveObj["targetCol"].toInt()
                };
                move.shootPos = {
                    moveObj["shootRow"].toInt(),
                    moveObj["shootCol"].toInt()
                };
                history.append(move);
            }
        }
        m_chessboard->m_history = history;
    }

    // 检查游戏是否结束
    bool gameOver = m_chessboard->checkGameOver();

    // 初始化 Bots
    if (!gameOver) {
        if (!m_chessboard->m_whiteIsPlayer) {
            m_whiteBot = new Bot(m_chessboard, true, whiteWeights, parent());
        }
        if (!m_chessboard->m_blackIsPlayer) {
            m_blackBot = new Bot(m_chessboard, false, blackWeights, parent());
        }
    }

    return true;
}
