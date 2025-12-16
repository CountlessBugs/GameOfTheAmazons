#ifndef SAVEGAME_H
#define SAVEGAME_H

#include <QObject>
#include "chessboard.h"
#include "bot.h"

class SaveGame : public QObject
{
    Q_OBJECT
public:
    explicit SaveGame(Chessboard *chessboard, Bot *whiteBot = nullptr, Bot *blackBot = nullptr, QObject *parent = nullptr);

    static QJsonArray boardToJsonArray(const Chessboard::Board &board);
    static Chessboard::Board jsonArrayToBoard(const QJsonArray &boardArray);

    bool saveGameAsJson();
    bool loadSave(QString filePath);

    QPair<Bot*, Bot*> getBots() const { return {m_whiteBot, m_blackBot}; }

private:
    Chessboard *m_chessboard;
    Bot *m_whiteBot;
    Bot *m_blackBot;

};

#endif // SAVEGAME_H
