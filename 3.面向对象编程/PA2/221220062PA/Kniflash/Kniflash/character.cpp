#include "character.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include <QDebug>

Character::Character(QGraphicsItem *parent)
    : QGraphicsObject(parent),
      m_health(100),
      m_maxHealth(100),
      m_knives(4),
      m_speed(2.0),
      m_alive(true),
      m_idleAnimation(nullptr),
      m_walkAnimation(nullptr),
      m_currentAnimation(nullptr),
      m_knifeRadius(50.0),
      m_rotationAngle(0.0),
      m_rotationSpeed(0.05) // 旋转速度可以根据需要调整
{
    // 初始化动画定时器
    connect(&m_animationTimer, &QTimer::timeout, this, &Character::advanceAnimation);
    m_animationTimer.start(50);
    
    // 初始化飞刀旋转定时器
    connect(&m_knifeRotationTimer, &QTimer::timeout, this, &Character::updateKnivesPosition);
    m_knifeRotationTimer.start(16); // ~60 FPS旋转动画
    
    // 创建飞刀
    createKnives();
}

Character::~Character()
{
    delete m_idleAnimation;
    delete m_walkAnimation;
    
    // 删除飞刀图形项
    qDeleteAll(m_knifeItems);
    m_knifeItems.clear();
}

QRectF Character::boundingRect() const
{
    // 根据当前帧的尺寸返回角色的包围矩形
    return QRectF(-m_currentFrame.width() / 2, -m_currentFrame.height() / 2,
                 m_currentFrame.width(), m_currentFrame.height());
}

void Character::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    // 绘制当前帧
    if (!m_currentFrame.isNull()) {
        painter->drawPixmap(-m_currentFrame.width() / 2, -m_currentFrame.height() / 2, m_currentFrame);
    }
    
    // 绘制血条
    painter->setPen(Qt::red);
    painter->setBrush(Qt::red);
    double healthBarWidth = 60.0;
    painter->drawRect(-healthBarWidth / 2, -60, healthBarWidth * m_health / m_maxHealth, 5);
    
    painter->setPen(Qt::black);
    painter->setBrush(Qt::transparent);
    painter->drawRect(-healthBarWidth / 2, -60, healthBarWidth, 5);
}

void Character::setHealth(int health)
{
    qDebug() << "Character::setHealth called, current:" << m_health << "new:" << health;
    
    // 检查是否已经死亡
    if (!m_alive) {
        qDebug() << "Character already dead, ignoring health change";
        return;
    }
    
    int oldHealth = m_health;
    
    // 处理生命值变化
    if (health <= 0) {
        qDebug() << "Character health <= 0, character will die";
        m_health = 0;
        m_alive = false;
        emit healthChanged(m_health);
        update();
        
        // 发射死亡信号并调用死亡方法
        QMetaObject::invokeMethod(this, "die", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, [this]() { emit died(this); }, Qt::QueuedConnection);
    } else if (health > m_maxHealth) {
        m_health = m_maxHealth;
        emit healthChanged(m_health);
        update();
    } else {
        m_health = health;
        emit healthChanged(m_health);
        update();
    }
    
    qDebug() << "Health changed from" << oldHealth << "to" << m_health;
}

void Character::setMaxHealth(int maxHealth)
{
    if (maxHealth > 0) {
        m_maxHealth = maxHealth;
        if (m_health > m_maxHealth) {
            m_health = m_maxHealth;
            emit healthChanged(m_health);
        }
        update();
    }
}

void Character::setKnives(int knives)
{
    if (knives >= 0) {
        m_knives = knives;
        updateKnives();
        emit knivesChanged(m_knives);
    }
}

void Character::setSpeed(double speed)
{
    if (speed > 0) {
        m_speed = speed;
    }
}

void Character::advanceAnimation()
{
    if (m_currentAnimation && m_currentAnimation->state() == QMovie::Running) {
        m_currentFrame = m_currentAnimation->currentPixmap().scaled(QSize(80, 80), Qt::KeepAspectRatio);
        m_currentAnimation->jumpToNextFrame();
        update();
    }
}

void Character::createKnives()
{
    // 删除现有的飞刀
    qDeleteAll(m_knifeItems);
    m_knifeItems.clear();
    
    // 创建新的飞刀
    for (int i = 0; i < m_knives; ++i) {
        // 使用QGraphicsPixmapItem
        QGraphicsPixmapItem* knife = new QGraphicsPixmapItem(this);
        
        // 载入飞刀图像
        QPixmap knifePixmap(":/images/knife.png");
        if (!knifePixmap.isNull()) {
            knifePixmap = knifePixmap.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            knife->setPixmap(knifePixmap);
            knife->setOffset(-knifePixmap.width()/2, -knifePixmap.height()/2); // 居中
        }
        
        m_knifeItems.append(knife);
    }
    
    updateKnives();
}

void Character::updateKnives()
{
    // 调整飞刀数量
    int currentCount = m_knifeItems.size();
    
    if (m_knives > currentCount) {
        // 添加新飞刀
        for (int i = currentCount; i < m_knives; ++i) {
            // 使用QGraphicsPixmapItem而不是QGraphicsEllipseItem
            QGraphicsPixmapItem* knife = new QGraphicsPixmapItem(this);
            
            // 载入飞刀图像
            QPixmap knifePixmap(":/images/knife.png");
            if (!knifePixmap.isNull()) {
                knifePixmap = knifePixmap.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                knife->setPixmap(knifePixmap);
                knife->setOffset(-knifePixmap.width()/2, -knifePixmap.height()/2); // 居中
            }
            
            m_knifeItems.append(knife);
        }
    } else if (m_knives < currentCount) {
        // 移除多余的飞刀
        for (int i = currentCount - 1; i >= m_knives; --i) {
            delete m_knifeItems.takeAt(i);
        }
    }
    
    // 更新飞刀位置和旋转角度
    // 飞刀半径随飞刀数量增加而增加，最小50，每把刀增加10
    m_knifeRadius = 50.0 + (m_knifeItems.size() * 10);
    
    // 更新飞刀位置
    updateKnivesPosition();
}

void Character::updateKnivesPosition()
{
    // 更新旋转角度
    m_rotationAngle += m_rotationSpeed;
    if (m_rotationAngle >= 2 * M_PI) {
        m_rotationAngle -= 2 * M_PI;
    }
    
    // 计算每把飞刀的位置
    double angleStep = 2 * M_PI / m_knifeItems.size();
    
    for (int i = 0; i < m_knifeItems.size(); ++i) {
        double angle = m_rotationAngle + (i * angleStep);
        double x = m_knifeRadius * qCos(angle);
        double y = m_knifeRadius * qSin(angle);
        m_knifeItems[i]->setPos(x, y);
        
        // 修改旋转角度，使飞刀上方朝外
        // 在Qt中，0度是指向右侧，顺时针旋转，所以要让刀尖朝外需要加90度
        m_knifeItems[i]->setRotation(angle * 180 / M_PI + 90);
    }
} 