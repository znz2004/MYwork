#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsView>
#include <QKeyEvent>
#include "gamescene.h"

class GameView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit GameView(QWidget *parent = nullptr);
    ~GameView();
    
    // 启动游戏
    void startGame();
    
signals:
    void gameExited();

public slots:
    void handleGameOver(bool victory, int kills, int surviveTime);
    
    // 在UI线程显示游戏结束画面
    Q_INVOKABLE void showGameOverScreen(bool victory, int kills, int surviveTime);

protected:
    // 键盘事件
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    GameScene* m_gameScene;
    
    // 更新视图
    void updateView();
};

#endif // GAMEVIEW_H 