#ifndef GAMESCENE_H
#define GAMESCENE_H

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QElapsedTimer>
#include <QList>
#include "player.h"
#include "enemy.h"
#include "item.h"

class GameScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit GameScene(QObject *parent = nullptr);
    ~GameScene();
    
    // 游戏初始化
    void initGame();
    void resetGame();
    
    // 获取玩家
    Player* getPlayer() const { return m_player; }

signals:
    void gameOver(bool victory, int kills, int surviveTime);

public slots:
    void update();
    void checkCollisions();
    void spawnItems();
    void handlePlayerDeath();
    void checkGameStatus();

private:
    // 更新目标指示器
    void updateTargetingIndicators();
    
    // 创建地图
    void createMap();
    
    // 创建树丛
    void createBushes(int count);
    
    // 创建玩家
    void createPlayer();
    
    // 创建敌人
    void createEnemies(int count);
    
    // 创建道具
    void createItems(int knifeCount, int heartCount, int bootsCount);
    
    // 选择随机地图位置
    QPointF randomMapPosition();
    
    // 游戏基本元素
    QGraphicsPixmapItem* m_background;
    QList<QGraphicsEllipseItem*> m_bushes;
    Player* m_player;
    QList<Enemy*> m_enemies;
    QList<Item*> m_items;
    
    // 定时器
    QTimer m_updateTimer;
    QTimer m_collisionTimer;
    QTimer m_itemSpawnTimer;
    QTimer m_gameStatusTimer;
    
    // 游戏数据
    int m_mapRadius;
    QElapsedTimer m_gameTime;
    int m_kills;
    bool m_gameActive;
    
    // 地图限制
    QGraphicsEllipseItem* m_mapBoundary;
    
    // 目标指示线列表
    QList<QGraphicsItem*> m_targetingLines;
};

#endif // GAMESCENE_H 