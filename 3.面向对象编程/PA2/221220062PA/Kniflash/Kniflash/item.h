#ifndef ITEM_H
#define ITEM_H

#include <QGraphicsPixmapItem>
#include <QGraphicsObject>
#include "player.h"

enum class ItemType {
    Knife,  // 飞刀
    Heart,  // 治愈之心
    Boots   // 疾步之靴
};

class Item : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit Item(ItemType type, QGraphicsItem *parent = nullptr);
    virtual ~Item();

    // 道具类型
    ItemType getType() const { return m_type; }
    
    // 道具效果
    virtual void applyEffect(Player* player);
    
    // 碰撞检测半径
    double getCollisionRadius() const { return m_collisionRadius; }
    
    // QGraphicsItem重写
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    ItemType m_type;
    QPixmap m_pixmap;
    double m_collisionRadius;
    bool m_collected;
};

#endif // ITEM_H 