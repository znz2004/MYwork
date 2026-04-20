#ifndef PLAYER_H
#define PLAYER_H

#include "character.h"
#include <QGraphicsItem>
#include <QKeyEvent>
#include <QSet>

class Player : public Character
{
    Q_OBJECT

public:
    explicit Player(QGraphicsItem *parent = nullptr);
    virtual ~Player();

    // 从Character继承的纯虚函数实现
    void move() override;
    void attack() override;
    void takeDamage(int damage) override;
    void die() override;

    // 处理按键事件
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    // 自动获取飞刀
    void startAutoKnifeCollection();
    void stopAutoKnifeCollection();

    // 道具效果
    void activateSpeedBoost();
    
    // 获取被锁定的敌人
    Character* getLockedTarget() const { return m_lockedTarget; }

    // 重置玩家状态
    void reset();

signals:
    void playerMoved();
    void playerAttacked();
    void playerDied();

private slots:
    void autoCollectKnife();
    void checkForTargets();
    void resetSpeedBoost();

private:
    // 按键状态
    QSet<int> m_pressedKeys;
    bool m_isMoving;
    
    // 速度提升
    bool m_hasSpeedBoost;
    double m_normalSpeed;
    QTimer m_speedBoostTimer;
    
    // 自动获取飞刀
    QTimer m_autoKnifeTimer;
    
    // 索敌
    QTimer m_targetCheckTimer;
    Character* m_lockedTarget;
    QGraphicsEllipseItem* m_targetIndicator;
    double m_targetingRadius;
    
    // 攻击
    QTimer m_attackTimer;
};

#endif // PLAYER_H 