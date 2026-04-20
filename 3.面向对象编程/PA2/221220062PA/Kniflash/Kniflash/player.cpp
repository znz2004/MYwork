#include "player.h"
#include "enemy.h"
#include <QPainter>
#include <QDebug>
#include <QtMath>
#include <QGraphicsScene>
#include <QGraphicsColorizeEffect>
#include <QPointer>

Player::Player(QGraphicsItem *parent)
    : Character(parent),
      m_isMoving(false),
      m_hasSpeedBoost(false),
      m_normalSpeed(2.0),
      m_lockedTarget(nullptr),
      m_targetingRadius(300.0)
{
    // 设置初始属性
    m_health = 100;
    m_maxHealth = 100;
    m_knives = 4;
    m_speed = m_normalSpeed;
    
    // 加载动画
    m_idleAnimation = new QMovie(":/images/player_idle.gif");
    m_walkAnimation = new QMovie(":/images/player_walk.gif");
    
    // 设置初始动画
    m_idleAnimation->start();
    m_walkAnimation->start();
    m_currentAnimation = m_idleAnimation;
    m_currentFrame = m_idleAnimation->currentPixmap().scaled(QSize(80, 80), Qt::KeepAspectRatio);
    
    // 初始化定时器
    connect(&m_autoKnifeTimer, &QTimer::timeout, this, &Player::autoCollectKnife);
    connect(&m_speedBoostTimer, &QTimer::timeout, this, &Player::resetSpeedBoost);
    connect(&m_targetCheckTimer, &QTimer::timeout, this, &Player::checkForTargets);
    
    // 创建目标指示器
    m_targetIndicator = new QGraphicsEllipseItem(-10, -10, 20, 20, this);
    m_targetIndicator->setBrush(QBrush(Qt::yellow));
    m_targetIndicator->setPen(QPen(Qt::black));
    m_targetIndicator->setVisible(false);
    
    // 设置为可获取焦点
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFocus();
    
    // 启动自动飞刀收集
    startAutoKnifeCollection();
    
    // 启动目标检测
    m_targetCheckTimer.start(200);
}

Player::~Player()
{
    delete m_targetIndicator;
}

void Player::move()
{
    if (m_pressedKeys.isEmpty()) {
        // 没有按键按下，设置为站立状态
        if (m_isMoving) {
            m_isMoving = false;
            m_currentAnimation = m_idleAnimation;
        }
        return;
    }
    
    // 计算移动方向
    qreal dx = 0;
    qreal dy = 0;
    
    if (m_pressedKeys.contains(Qt::Key_W) || m_pressedKeys.contains(Qt::Key_Up)) {
        dy -= m_speed;
    }
    if (m_pressedKeys.contains(Qt::Key_S) || m_pressedKeys.contains(Qt::Key_Down)) {
        dy += m_speed;
    }
    if (m_pressedKeys.contains(Qt::Key_A) || m_pressedKeys.contains(Qt::Key_Left)) {
        dx -= m_speed;
    }
    if (m_pressedKeys.contains(Qt::Key_D) || m_pressedKeys.contains(Qt::Key_Right)) {
        dx += m_speed;
    }
    
    // 对斜向移动进行归一化处理
    if (dx != 0 && dy != 0) {
        qreal length = qSqrt(dx * dx + dy * dy);
        dx = dx / length * m_speed;
        dy = dy / length * m_speed;
    }
    
    // 如果有移动，设置为行走状态
    if (dx != 0 || dy != 0) {
        if (!m_isMoving) {
            m_isMoving = true;
            m_currentAnimation = m_walkAnimation;
        }
        
        // 更新位置
        moveBy(dx, dy);
        emit playerMoved();
    }
}

void Player::attack()
{
    // 检查是否有目标和足够的飞刀
    if (!m_lockedTarget) {
        qDebug() << "Attack failed: No target locked";
        return;
    }
    
    if (m_knives <= 0) {
        qDebug() << "Attack failed: No knives available";
        return;
    }
    
    if (!m_lockedTarget->isAlive()) {
        qDebug() << "Attack failed: Target is not alive";
        return;
    }
    
    qDebug() << "Player attacking target at position:" << m_lockedTarget->scenePos();
    
    // 减少飞刀数量
    setKnives(m_knives - 1);
    
    // 创建飞行的飞刀视觉效果
    QGraphicsPixmapItem* flyingKnife = new QGraphicsPixmapItem();
    QPixmap knifePixmap(":/images/knife.png");
    
    if (!knifePixmap.isNull()) {
        knifePixmap = knifePixmap.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        flyingKnife->setPixmap(knifePixmap);
        flyingKnife->setOffset(-knifePixmap.width()/2, -knifePixmap.height()/2);
        
        // 计算方向角度 - 从玩家位置到目标位置的向量
        QPointF targetScenePos = m_lockedTarget->scenePos();
        QPointF playerScenePos = scenePos();
        QPointF direction = targetScenePos - playerScenePos;
        qreal angle = qAtan2(direction.y(), direction.x());
        
        // 设置旋转 - 加90度使刀尖朝向目标
        flyingKnife->setRotation(angle * 180 / M_PI + 90);
        
        // 添加到场景
        scene()->addItem(flyingKnife);
        flyingKnife->setPos(scenePos());
        
        // 创建动画计时器
        QTimer* flyTimer = new QTimer();
        QPointF startPos = playerScenePos;
        int animationDuration = 500; // 飞行时间，毫秒
        int steps = 20; // 动画步数
        int interval = animationDuration / steps;
        
        // 连接计时器到匿名函数进行动画
        int currentStep = 0;
        QObject* context = new QObject();
        connect(flyTimer, &QTimer::timeout, context, [=]() mutable {
            // 是否移除飞刀
            bool shouldRemove = false;
            
            // 检查目标是否还存在
            if (currentStep >= steps) {
                qDebug() << "Knife animation completed";
                shouldRemove = true;
            } else if (!m_lockedTarget) {
                qDebug() << "Target no longer exists";
                shouldRemove = true;
            } else if (!m_lockedTarget->isAlive()) {
                qDebug() << "Target is no longer alive";
                shouldRemove = true;
            }
            
            if (shouldRemove) {
                // 移除飞刀并清理资源
                scene()->removeItem(flyingKnife);
                delete flyingKnife;
                delete flyTimer;
                delete context;
                return;
            }
            
            // 获取当前目标位置（允许目标移动）
            QPointF currentTargetPos = m_lockedTarget->scenePos();
            
            // 计算飞刀当前位置 - 线性插值
            qreal progress = static_cast<qreal>(currentStep) / steps;
            QPointF newPos = startPos + (currentTargetPos - startPos) * progress;
            flyingKnife->setPos(newPos);
            
            // 如果到达最后一步，造成伤害
            if (currentStep == steps - 1) {
                qDebug() << "Knife hit target. Dealing damage.";
                m_lockedTarget->takeDamage(10);
            }
            
            currentStep++;
        });
        
        // 启动动画计时器
        flyTimer->start(interval);
    } else {
        qDebug() << "Failed to load knife pixmap";
    }
    
    emit playerAttacked();
}

void Player::takeDamage(int damage)
{
    qDebug() << "Player taking damage:" << damage << "Current health:" << m_health;
    
    // 玩家受伤效果
    setHealth(m_health - damage);
    
    // 如果玩家已经死亡，直接返回不添加视觉效果
    if (!m_alive) {
        qDebug() << "Player is dead, skipping damage visual effect";
        return;
    }
    
    // 闪烁效果
    QGraphicsColorizeEffect* effect = new QGraphicsColorizeEffect();
    effect->setColor(Qt::red);
    setGraphicsEffect(effect);
    
    // 创建一个独立的QObject作为effect的父对象，确保效果被正确清理
    QObject* context = new QObject();
    effect->setParent(context);
    
    // 使用QPointer监控this对象的有效性
    QPointer<Player> selfPointer(this);
    
    // 使用context作为QTimer的父对象，确保timer在context被删除时也被删除
    QTimer* timer = new QTimer(context);
    timer->setSingleShot(true);
    
    // 连接计时器到匿名函数
    QObject::connect(timer, &QTimer::timeout, context, [selfPointer, effect, context]() {
        // 检查玩家对象是否仍然有效
        if (selfPointer) {
            // 玩家对象有效，正常移除效果
            selfPointer->setGraphicsEffect(nullptr);
            qDebug() << "Damage effect removed. Health now:" << selfPointer->getHealth();
        }
        
        // 无论如何都删除context（这将同时删除effect）
        context->deleteLater();
    });
    
    // 启动计时器
    timer->start(500);
}

void Player::die()
{
    qDebug() << "Player::die() called";
    
    // 防止重复调用
    if (!m_alive) {
        qDebug() << "Player already dead, ignoring die() call";
        return;
    }
    
    // 播放死亡动画
    // 这里简化，只是停止移动和攻击
    m_alive = false;
    
    qDebug() << "Player marked as dead, stopping timers";
    
    // 停止所有计时器
    stopAutoKnifeCollection();
    m_targetCheckTimer.stop();
    m_speedBoostTimer.stop();
    
    // 清除目标
    m_lockedTarget = nullptr;
    m_targetIndicator->setVisible(false);
    
    // 设置透明度降低以视觉上表示死亡
    setOpacity(0.5);
    
    // 通知游戏结束
    qDebug() << "Emitting playerDied signal";
    emit playerDied();
}

void Player::keyPressEvent(QKeyEvent *event)
{
    m_pressedKeys.insert(event->key());
    
    // 空格键攻击
    if (event->key() == Qt::Key_Space) {
        attack();
    }
    
    // 继续传递事件
    QGraphicsItem::keyPressEvent(event);
}

void Player::keyReleaseEvent(QKeyEvent *event)
{
    m_pressedKeys.remove(event->key());
    
    // 继续传递事件
    QGraphicsItem::keyReleaseEvent(event);
}

void Player::startAutoKnifeCollection()
{
    m_autoKnifeTimer.start(1000); // 每秒尝试获取一把飞刀
}

void Player::stopAutoKnifeCollection()
{
    m_autoKnifeTimer.stop();
}

void Player::activateSpeedBoost()
{
    // 保存原速度并增加速度
    m_normalSpeed = m_speed;
    m_speed = m_normalSpeed * 1.5;
    m_hasSpeedBoost = true;
    
    // 设置5秒后恢复
    m_speedBoostTimer.start(5000);
}

void Player::reset()
{
    // 重置玩家状态
    m_health = m_maxHealth;
    m_knives = 4;
    m_speed = m_normalSpeed;
    m_alive = true;
    m_hasSpeedBoost = false;
    m_lockedTarget = nullptr;
    m_targetIndicator->setVisible(false);
    m_pressedKeys.clear();
    m_isMoving = false;
    m_currentAnimation = m_idleAnimation;
    
    updateKnives();
    startAutoKnifeCollection();
    m_targetCheckTimer.start(200);
    
    setFocus();
}

void Player::autoCollectKnife()
{
    // 当飞刀数量小于4时，自动增加一把
    if (m_knives < 4) {
        setKnives(m_knives + 1);
    }
}

void Player::checkForTargets()
{
    if (!scene() || !isAlive()) {
        // 清除之前的目标
        if (m_lockedTarget) {
            qDebug() << "Clearing target because player is not in scene or not alive";
            m_lockedTarget = nullptr;
            m_targetIndicator->setVisible(false);
        }
        return;
    }
    
    // 首先验证当前目标是否还有效
    if (m_lockedTarget) {
        if (!m_lockedTarget->isAlive() || !m_lockedTarget->scene()) {
            qDebug() << "Current target is no longer valid (dead or removed from scene)";
            m_lockedTarget = nullptr;
            m_targetIndicator->setVisible(false);
        }
    }
    
    // 更新索敌范围，基础范围300，每把刀增加30的范围
    m_targetingRadius = 300.0 + (m_knives * 30.0);
    
    // 搜索圆形区域内的所有项
    QList<QGraphicsItem*> nearbyItems = scene()->items(
        QRectF(scenePos().x() - m_targetingRadius, 
               scenePos().y() - m_targetingRadius,
               m_targetingRadius * 2, 
               m_targetingRadius * 2),
        Qt::IntersectsItemShape
    );
    
    Character* closestEnemy = nullptr;
    qreal closestDistance = m_targetingRadius;
    
    for (QGraphicsItem* item : nearbyItems) {
        if (item == this) continue; // 跳过自己
        
        Enemy* enemy = dynamic_cast<Enemy*>(item);
        if (enemy && enemy->isAlive() && enemy->scene()) {
            qreal distance = QLineF(scenePos(), enemy->scenePos()).length();
            if (distance < closestDistance) {
                closestDistance = distance;
                closestEnemy = enemy;
            }
        }
    }
    
    // 如果找到有效敌人，标记为目标
    if (closestEnemy) {
        // 如果是新目标，输出调试信息
        if (m_lockedTarget != closestEnemy) {
            qDebug() << "New target locked. Distance:" << closestDistance;
            m_lockedTarget = closestEnemy;
        }
        
        m_targetIndicator->setVisible(true);
        
        // 更新目标指示器位置到目标位置
        QPointF targetPos = mapFromScene(m_lockedTarget->scenePos());
        m_targetIndicator->setPos(targetPos);
        
        // 使目标指示器大小更明显
        if (m_targetIndicator->rect().width() != 30) {
            m_targetIndicator->setRect(-15, -15, 30, 30);
        }
    } else if (m_lockedTarget != nullptr) {
        // 如果之前有目标但现在没有了，输出调试信息
        qDebug() << "Target lost - no enemies in range";
        m_lockedTarget = nullptr;
        m_targetIndicator->setVisible(false);
    }
}

void Player::resetSpeedBoost()
{
    m_speed = m_normalSpeed;
    m_hasSpeedBoost = false;
    m_speedBoostTimer.stop();
} 