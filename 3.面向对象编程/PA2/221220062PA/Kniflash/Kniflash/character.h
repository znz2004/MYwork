#ifndef CHARACTER_H
#define CHARACTER_H

#include <QGraphicsObject>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QMovie>
#include <QTimer>
#include <QPointF>

class Character : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit Character(QGraphicsItem *parent = nullptr);
    virtual ~Character();

    // 基本属性
    int getHealth() const { return m_health; }
    void setHealth(int health);
    int getMaxHealth() const { return m_maxHealth; }
    void setMaxHealth(int maxHealth);
    int getKnives() const { return m_knives; }
    void setKnives(int knives);
    double getSpeed() const { return m_speed; }
    void setSpeed(double speed);
    bool isAlive() const { return m_alive; }
    
    // 获取飞刀半径
    double getKnifeRadius() const { return m_knifeRadius; }

    // 纯虚函数，子类必须实现
    virtual void move() = 0;
    virtual void attack() = 0;
    virtual void takeDamage(int damage) = 0;
    virtual void die() = 0;

    // QGraphicsItem虚函数实现
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void died(Character* character);
    void healthChanged(int newHealth);
    void knivesChanged(int newKnives);

protected:
    void advanceAnimation();
    void createKnives();
    void updateKnives();
    void updateKnivesPosition();

    // 基础属性
    int m_health;
    int m_maxHealth;
    int m_knives;
    double m_speed;
    bool m_alive;

    // 动画相关
    QMovie* m_idleAnimation;
    QMovie* m_walkAnimation;
    QMovie* m_currentAnimation;
    QPixmap m_currentFrame;
    QTimer m_animationTimer;
    
    // 飞刀相关
    QList<QGraphicsPixmapItem*> m_knifeItems;
    double m_knifeRadius;
    double m_rotationAngle;
    double m_rotationSpeed;
    QTimer m_knifeRotationTimer;
};

#endif // CHARACTER_H 