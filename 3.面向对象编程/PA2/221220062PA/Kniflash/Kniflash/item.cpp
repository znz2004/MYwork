#include "item.h"
#include <QPainter>
#include <QDebug>
#include <QGraphicsScene>

Item::Item(ItemType type, QGraphicsItem *parent)
    : QGraphicsObject(parent),
      m_type(type),
      m_collisionRadius(30.0),
      m_collected(false)
{
    // 根据道具类型设置图像
    switch (type) {
        case ItemType::Knife:
            m_pixmap.load(":/images/knife.png");
            break;
        case ItemType::Heart:
            m_pixmap.load(":/images/heart.png");
            break;
        case ItemType::Boots:
            m_pixmap.load(":/images/boots.png");
            break;
    }
    
    // 调整图像大小
    if (!m_pixmap.isNull()) {
        m_pixmap = m_pixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
}

Item::~Item()
{
}

void Item::applyEffect(Player* player)
{
    if (!player || m_collected) return;
    
    m_collected = true;
    
    // 根据道具类型应用效果
    switch (m_type) {
        case ItemType::Knife:
            // 增加飞刀
            player->setKnives(player->getKnives() + 1);
            break;
        case ItemType::Heart:
            // 增加生命值
            player->setHealth(player->getHealth() + 20);
            break;
        case ItemType::Boots:
            // 增加速度
            player->activateSpeedBoost();
            break;
    }
    
    // 道具被收集后删除
    if (scene()) {
        scene()->removeItem(this);
    }
    deleteLater();
}

QRectF Item::boundingRect() const
{
    return QRectF(-m_pixmap.width() / 2, -m_pixmap.height() / 2,
                 m_pixmap.width(), m_pixmap.height());
}

void Item::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    if (!m_pixmap.isNull()) {
        painter->drawPixmap(-m_pixmap.width() / 2, -m_pixmap.height() / 2, m_pixmap);
    }
} 