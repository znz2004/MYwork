#include "enemy.h"
#include "player.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsColorizeEffect>
#include <QDebug>
#include <QPointer>
#include <QElapsedTimer>

Enemy::Enemy(QGraphicsItem *parent)
    : Character(parent),
      m_targetingRadius(200.0),
      m_state(EnemyState::Wandering),
      m_lockedTarget(nullptr),
      m_attackReady(false),
      m_lastAttackTime(0)
{
    // 设置基础属性
    m_health = 50; // 敌人生命值设置为50
    m_maxHealth = 50;
    m_knives = 4;
    m_speed = 1.0; // 敌人移动速度比玩家慢
    
    // 加载动画 (使用与player相同的动画但尺寸更小)
    m_idleAnimation = new QMovie(":/images/player_idle.gif");
    m_walkAnimation = new QMovie(":/images/player_walk.gif");
    
    // 设置初始动画
    m_idleAnimation->start();
    m_walkAnimation->start();
    m_currentAnimation = m_walkAnimation; // 敌人始终处于行走状态
    m_currentFrame = m_walkAnimation->currentPixmap().scaled(QSize(60, 60), Qt::KeepAspectRatio);
    
    // 创建目标指示器 (与锁定的目标之间的连线)
    m_targetIndicator = new QGraphicsEllipseItem(-10, -10, 20, 20, this);
    m_targetIndicator->setBrush(QBrush(Qt::red)); // 红色指示器
    m_targetIndicator->setPen(QPen(Qt::black, 1));
    m_targetIndicator->setVisible(false); // 初始时不显示
    m_targetIndicator->setZValue(300); // 确保在所有元素上层显示
    
    // 初始化定时器
    connect(&m_stateTimer, &QTimer::timeout, this, &Enemy::updateState);
    connect(&m_attackTimer, &QTimer::timeout, this, &Enemy::prepareFire);
    
    // 启动状态更新
    m_stateTimer.start(3000); // 每3秒更新一次状态
    m_attackTimer.start(1000); // 每秒检查是否可以攻击
    
    // 设置随机移动方向
    changeDirection();
    
    qDebug() << "Enemy created at position:" << pos();
}

Enemy::~Enemy()
{
    // 清理资源
    delete m_targetIndicator; // 释放目标指示器
}

void Enemy::move()
{
    if (!m_alive) return;
    
    // 更新位置
    if (m_state == EnemyState::Wandering) {
        // 随机移动
        moveBy(m_moveDirX * m_speed, m_moveDirY * m_speed);
    } else if (m_state == EnemyState::Targeting) {
        // 已锁定目标，但不主动靠近
        // 实际上，敌人可以继续随机移动
        moveBy(m_moveDirX * m_speed, m_moveDirY * m_speed);
    }
}

void Enemy::attack()
{
    // 每秒尝试发射一次飞刀
    if (!m_attackReady || !m_lockedTarget || !m_lockedTarget->isAlive() || m_knives <= 0) {
        return;
    }
    
    // 重置攻击准备状态
    m_attackReady = false;
    
    qDebug() << "Enemy attacking target at position:" << m_lockedTarget->scenePos();
    
    // 减少飞刀数量
    setKnives(m_knives - 1);
    
    // 创建飞行的飞刀视觉效果
    QGraphicsPixmapItem* flyingKnife = new QGraphicsPixmapItem();
    QPixmap knifePixmap(":/images/knife.png");
    
    if (!knifePixmap.isNull()) {
        knifePixmap = knifePixmap.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        flyingKnife->setPixmap(knifePixmap);
        flyingKnife->setOffset(-knifePixmap.width()/2, -knifePixmap.height()/2);
        
        // 计算方向角度
        QPointF targetScenePos = m_lockedTarget->scenePos();
        QPointF enemyScenePos = scenePos();
        QPointF direction = targetScenePos - enemyScenePos;
        qreal angle = qAtan2(direction.y(), direction.x());
        
        // 设置旋转
        flyingKnife->setRotation(angle * 180 / M_PI + 90);
        
        // 添加到场景
        scene()->addItem(flyingKnife);
        flyingKnife->setPos(scenePos());
        
        // 创建动画计时器
        QTimer* flyTimer = new QTimer();
        QPointF startPos = scenePos();
        int animationDuration = 500; // 飞行时间，毫秒
        int steps = 20; // 动画步数
        int interval = animationDuration / steps;
        
        // 连接计时器到匿名函数进行动画
        int currentStep = 0;
        QObject* context = new QObject();
        connect(flyTimer, &QTimer::timeout, context, [=]() mutable {
            // 是否需要移除飞刀
            bool shouldRemove = false;
            
            if (currentStep >= steps) {
                qDebug() << "Enemy knife animation completed";
                shouldRemove = true;
            } else if (!m_lockedTarget) {
                qDebug() << "Enemy target no longer exists";
                shouldRemove = true;
            } else if (!m_lockedTarget->isAlive()) {
                qDebug() << "Enemy target is no longer alive";
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
            
            // 获取当前目标位置
            QPointF currentTargetPos = m_lockedTarget->scenePos();
            
            // 计算飞刀当前位置
            qreal progress = static_cast<qreal>(currentStep) / steps;
            QPointF newPos = startPos + (currentTargetPos - startPos) * progress;
            flyingKnife->setPos(newPos);
            
            // 如果到达最后一步，造成伤害
            if (currentStep == steps - 1) {
                qDebug() << "Enemy knife hit target. Dealing damage.";
                m_lockedTarget->takeDamage(3);
            }
            
            currentStep++;
        });
        
        // 启动动画计时器
        flyTimer->start(interval);
    }
}

void Enemy::takeDamage(int damage)
{
    qDebug() << "Enemy taking damage:" << damage << "Current health:" << m_health;
    
    // 设置新的生命值
    setHealth(m_health - damage);
    
    // 如果敌人已经死亡，直接返回不添加视觉效果
    if (!m_alive) {
        qDebug() << "Enemy is dead, skipping damage visual effect";
        return;
    }
    
    // 受伤特效
    QGraphicsColorizeEffect* effect = new QGraphicsColorizeEffect();
    effect->setColor(Qt::red);
    setGraphicsEffect(effect);
    
    // 创建一个独立的QObject作为effect的父对象，确保效果被正确清理
    QObject* context = new QObject();
    effect->setParent(context);
    
    // 使用QPointer监控this对象的有效性
    QPointer<Enemy> selfPointer(this);
    
    // 使用context作为QTimer的父对象，确保timer在context被删除时也被删除
    QTimer* timer = new QTimer(context);
    timer->setSingleShot(true);
    
    // 连接计时器到匿名函数
    QObject::connect(timer, &QTimer::timeout, context, [selfPointer, effect, context]() {
        // 检查敌人对象是否仍然有效
        if (selfPointer) {
            // 敌人对象有效，正常移除效果
            selfPointer->setGraphicsEffect(nullptr);
            qDebug() << "Enemy damage effect removed. Health now:" << selfPointer->getHealth();
        }
        
        // 无论如何都删除context（这将同时删除effect）
        context->deleteLater();
    });
    
    // 启动计时器
    timer->start(500);
}

void Enemy::die()
{
    // 防止重复调用
    if (!m_alive) return;
    
    m_alive = false;
    qDebug() << "Enemy died at position:" << pos();
    
    // 停止所有定时器
    m_stateTimer.stop();
    m_attackTimer.stop();
    m_knifeRotationTimer.stop(); // 停止飞刀旋转定时器
    
    // 清除目标
    m_lockedTarget = nullptr;
    if (m_targetIndicator) {
        m_targetIndicator->setVisible(false);
    }
    
    // 设置状态为死亡
    m_state = EnemyState::Dead;
    
    // 立即隐藏所有飞刀视觉效果
    for (QGraphicsPixmapItem* knife : m_knifeItems) {
        if (knife) {
            knife->hide();  // 使用hide()完全隐藏飞刀
            knife->setParentItem(nullptr); // 解除父子关系，防止自动显示
        }
    }
    // 清除飞刀
    qDeleteAll(m_knifeItems); // 删除所有飞刀对象
    m_knifeItems.clear();     // 清空飞刀列表
    setKnives(0);
    
    // 创建一个临时的消失动画
    if (scene()) {
        QGraphicsEllipseItem* fadeEffect = new QGraphicsEllipseItem(-30, -30, 60, 60);
        fadeEffect->setBrush(QBrush(QColor(255, 0, 0, 100)));
        fadeEffect->setPen(QPen(Qt::transparent));
        fadeEffect->setPos(pos());
        scene()->addItem(fadeEffect);
        
        // 完全隐藏敌人本身
        hide();  // 隐藏敌人主体
        
        // 从场景中移除敌人
        scene()->removeItem(this);
        
        // 创建一个QObject上下文来处理fadeEffect的生命周期
        QObject* context = new QObject();
        
        // 500毫秒后移除消失效果
        QTimer::singleShot(500, context, [context, fadeEffect]() {
            if (fadeEffect->scene()) {
                fadeEffect->scene()->removeItem(fadeEffect);
                delete fadeEffect;
            }
            context->deleteLater();
        });
    } else {
        // 如果没有场景，直接隐藏自己
        hide();
    }
    
    // 触发死亡信号
    emit died(this);
    
    // 使用deleteLater而不是直接删除，确保信号处理完毕后再删除
    deleteLater();
}

void Enemy::updateState()
{
    if (!m_alive || !scene()) return;
    
    // 检查是否有目标
    findTarget();
    
    // 如果有目标，设置状态为瞄准
    if (m_lockedTarget && m_lockedTarget->isAlive()) {
        m_state = EnemyState::Targeting;
    } else {
        m_state = EnemyState::Wandering;
        changeDirection(); // 改变随机移动方向
    }
}

void Enemy::findTarget()
{
    if (!scene() || !m_alive) return;
    
    // 获取附近的角色
    QList<QGraphicsItem*> nearbyItems = scene()->items(QRectF(pos().x() - m_targetingRadius, 
                                                   pos().y() - m_targetingRadius,
                                                   m_targetingRadius * 2, 
                                                   m_targetingRadius * 2),
                                                Qt::IntersectsItemShape);
    
    Player* closestPlayer = nullptr;
    qreal closestDistance = m_targetingRadius;
    
    for (QGraphicsItem* item : nearbyItems) {
        if (item == this) continue; // 跳过自己
        
        Player* player = dynamic_cast<Player*>(item);
        if (player && player->isAlive()) {
            qreal distance = QLineF(pos(), player->pos()).length();
            if (distance < closestDistance) {
                closestDistance = distance;
                closestPlayer = player;
            }
        }
    }
    
    // 如果找到玩家，锁定
    if (closestPlayer) {
        if (m_lockedTarget != closestPlayer) {
            qDebug() << "Enemy locked onto player. Distance:" << closestDistance;
        }
        m_lockedTarget = closestPlayer;
        
        // 更新目标指示器
        // 不在敌人自身显示指示器，而是在正式版中移除这段代码
        // m_targetIndicator->setVisible(true);
        
        // 目标的相对位置
        // QPointF targetPos = mapFromScene(m_lockedTarget->scenePos());
        // m_targetIndicator->setPos(targetPos);
    } else {
        if (m_lockedTarget) {
            qDebug() << "Enemy lost target";
        }
        m_lockedTarget = nullptr;
        m_targetIndicator->setVisible(false);
    }
}

void Enemy::prepareFire()
{
    // 确保角色是存活状态
    if (!m_alive || m_state == EnemyState::Dead) {
        return;
    }
    
    // 限制攻击频率 - 每秒最多1次
    QElapsedTimer currentTime;
    currentTime.start();
    qint64 currentTimeMs = currentTime.msecsSinceReference();
    
    // 如果攻击间隔不足1秒，跳过
    if (m_lastAttackTime > 0 && currentTimeMs - m_lastAttackTime < 1000) {
        return;
    }
    
    // 每秒增加一个飞刀，最多4把
    if (m_knives < 4) {
        setKnives(m_knives + 1);
    }
    
    // 标记可以攻击
    m_attackReady = true;
    
    // 如果锁定了目标，攻击
    if (m_lockedTarget && m_lockedTarget->isAlive() && m_state == EnemyState::Targeting) {
        // 远程攻击 - 每秒最多1次
        attack();
        m_lastAttackTime = currentTimeMs;
    }
}

void Enemy::changeDirection()
{
    // 生成随机角度
    qreal angle = QRandomGenerator::global()->bounded(2.0 * M_PI);
    m_moveDirX = qCos(angle);
    m_moveDirY = qSin(angle);
} 