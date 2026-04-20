#ifndef ENEMY_H
#define ENEMY_H

#include "character.h"
#include <QGraphicsItem>
#include <QTimer>

// 前向声明Player类避免循环包含
class Player;

// 敌人状态枚举
enum class EnemyState {
    Wandering,  // 随机游荡
    Targeting,  // 锁定目标
    Dead        // 死亡状态
};

class Enemy : public Character
{
    Q_OBJECT
    
public:
    explicit Enemy(QGraphicsItem *parent = nullptr);
    virtual ~Enemy();
    
    // 从Character继承的纯虚函数实现
    void move() override;
    void attack() override;
    void takeDamage(int damage) override;
    void die() override;
    
    // 获取和设置目标
    void setTarget(Character* target);
    Character* getTarget() const { return m_lockedTarget; }
    
    // 随机移动
    void startRandomMovement();
    void stopRandomMovement();
    
    // 获取当前状态
    EnemyState getState() const { return m_state; }

private slots:
    // AI状态更新
    void updateState();
    
    // 寻找目标
    void findTarget();
    
    // 准备攻击
    void prepareFire();

private:
    // 改变移动方向
    void changeDirection();
    
    // 移动方向
    double m_moveDirX;
    double m_moveDirY;
    
    // 索敌范围
    double m_targetingRadius;
    
    // 当前状态
    EnemyState m_state;
    
    // 锁定的目标
    Character* m_lockedTarget;
    
    // 目标指示器
    QGraphicsEllipseItem* m_targetIndicator;
    
    // 攻击相关
    bool m_attackReady;
    qint64 m_lastAttackTime;
    
    // 定时器
    QTimer m_stateTimer;
    QTimer m_attackTimer;
};

#endif // ENEMY_H 