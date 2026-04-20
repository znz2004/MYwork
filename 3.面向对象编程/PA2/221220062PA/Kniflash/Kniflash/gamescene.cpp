#include "gamescene.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QDebug>
#include <QtMath>
#include <QGraphicsBlurEffect>
#include <QElapsedTimer>
#include <QGraphicsDropShadowEffect>

GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent),
      m_background(nullptr),
      m_player(nullptr),
      m_mapRadius(1200),
      m_kills(0),
      m_gameActive(false)
{
    // 连接定时器
    connect(&m_updateTimer, &QTimer::timeout, this, &GameScene::update);
    connect(&m_collisionTimer, &QTimer::timeout, this, &GameScene::checkCollisions);
    connect(&m_itemSpawnTimer, &QTimer::timeout, this, &GameScene::spawnItems);
    connect(&m_gameStatusTimer, &QTimer::timeout, this, &GameScene::checkGameStatus);
    
    qDebug() << "GameScene initialized";
}

GameScene::~GameScene()
{
    // 清理资源
    delete m_background;
    delete m_player;
    
    // 清理敌人
    qDeleteAll(m_enemies);
    m_enemies.clear();
    
    // 清理道具
    qDeleteAll(m_items);
    m_items.clear();
    
    // 清理树丛
    qDeleteAll(m_bushes);
    m_bushes.clear();
    
    // 清理目标指示线
    qDeleteAll(m_targetingLines);
    m_targetingLines.clear();
}

void GameScene::initGame()
{
    // 创建游戏地图和元素
    createMap();
    createBushes(5);
    createPlayer();
    createEnemies(5);
    createItems(10, 2, 2);
    
    // 启动游戏定时器
    m_updateTimer.start(16);  // 约60FPS
    m_collisionTimer.start(100);
    m_itemSpawnTimer.start(5000);  // 每5秒尝试生成新道具
    m_gameStatusTimer.start(1000);  // 每秒检查游戏状态
    
    // 重置游戏数据
    m_kills = 0;
    m_gameActive = true;
    m_gameTime.start();
    
    // 设置场景大小
    setSceneRect(-m_mapRadius, -m_mapRadius, 2 * m_mapRadius, 2 * m_mapRadius);
}

void GameScene::resetGame()
{
    // 停止所有定时器
    m_updateTimer.stop();
    m_collisionTimer.stop();
    m_itemSpawnTimer.stop();
    m_gameStatusTimer.stop();
    
    // 清理场景
    clear();
    
    // 重置游戏状态
    m_player = nullptr;
    m_background = nullptr;
    m_kills = 0;
    m_gameActive = false;
    m_enemies.clear();
    m_items.clear();
    m_bushes.clear();
    m_targetingLines.clear();
    
    // 重新初始化游戏
    initGame();
}

void GameScene::update()
{
    if (!m_gameActive || !m_player) return;
    
    // 更新玩家移动
    m_player->move();
    
    // 更新敌人移动
    for (Enemy* enemy : m_enemies) {
        if (enemy->isAlive()) {
            enemy->move();
        }
    }
    
    // 更新目标指示和锁定线
    updateTargetingIndicators();
}

void GameScene::updateTargetingIndicators()
{
    if (!m_player || !m_player->isAlive()) return;
    
    // 移除旧的锁定线
    for (QGraphicsItem* indicator : m_targetingLines) {
        removeItem(indicator);
        delete indicator;
    }
    m_targetingLines.clear();
    
    // 处理玩家的目标锁定线
    Character* playerTarget = m_player->getLockedTarget();
    if (playerTarget && playerTarget->isAlive()) {
        // 创建一个玩家到目标的指示线
        QLineF targetLine(m_player->pos(), playerTarget->pos());
        QGraphicsLineItem* lineItem = new QGraphicsLineItem(targetLine);
        
        // 设置线条样式 - 使用橙色虚线
        QPen linePen(QColor(255, 165, 0, 180), 2, Qt::DashLine);
        lineItem->setPen(linePen);
        lineItem->setZValue(250); // 确保在游戏对象上方，但在UI下方
        
        // 添加到场景
        addItem(lineItem);
        m_targetingLines.append(lineItem);
        
        // 在目标脚下添加标记
        QGraphicsEllipseItem* targetMarker = new QGraphicsEllipseItem(-15, -15, 30, 30);
        targetMarker->setBrush(QBrush(QColor(255, 165, 0, 150)));
        targetMarker->setPen(QPen(Qt::black, 1));
        targetMarker->setPos(playerTarget->pos().x(), playerTarget->pos().y() + 40); // 放在目标脚下
        targetMarker->setZValue(251); // 比线高一层
        
        // 添加到场景和列表
        addItem(targetMarker);
        m_targetingLines.append(targetMarker);
    }
    
    // 可选：显示敌人锁定玩家的线
    for (Enemy* enemy : m_enemies) {
        if (enemy->isAlive() && enemy->getTarget() == m_player && enemy->getState() == EnemyState::Targeting) {
            // 创建一个敌人到玩家的指示线
            QLineF targetLine(enemy->pos(), m_player->pos());
            QGraphicsLineItem* lineItem = new QGraphicsLineItem(targetLine);
            
            // 设置线条样式 - 使用红色虚线
            QPen linePen(QColor(255, 0, 0, 150), 1, Qt::DotLine);
            lineItem->setPen(linePen);
            lineItem->setZValue(249); // 确保在玩家锁定线下方
            
            // 添加到场景
            addItem(lineItem);
            m_targetingLines.append(lineItem);
        }
    }
}

void GameScene::checkCollisions()
{
    if (!m_gameActive || !m_player) return;
    
    // 检测玩家与道具的碰撞
    for (int i = m_items.size() - 1; i >= 0; --i) {
        Item* item = m_items[i];
        QLineF line(m_player->pos(), item->pos());
        if (line.length() < item->getCollisionRadius()) {
            // 应用道具效果
            qDebug() << "Player picked up item:" << static_cast<int>(item->getType());
            item->applyEffect(m_player);
            m_items.removeAt(i);
        }
    }
    
    // 检测玩家与敌人的碰撞
    for (Enemy* enemy : m_enemies) {
        if (enemy && enemy->isAlive()) {
            QLineF line(m_player->pos(), enemy->pos());
            double collisionDistance = 40; // 碰撞半径
            
            if (line.length() < collisionDistance) {
                qDebug() << "Player collided with enemy. Distance:" << line.length();
                
                // 玩家与敌人发生碰撞，将它们分开，但不造成伤害
                QPointF direction = line.unitVector().p2() - line.p1();
                double overlap = collisionDistance - line.length();
                
                // 将玩家稍微推开
                m_player->setPos(m_player->pos() - direction * overlap * 0.5);
                
                // 将敌人稍微推开
                enemy->setPos(enemy->pos() + direction * overlap * 0.5);
            }
            
            // 近身攻击判定
            // 有效攻击距离 - 取决于角色的飞刀环绕半径
            double playerKnifeRadius = m_player->getKnifeRadius();
            double enemyKnifeRadius = enemy->getKnifeRadius();
            
            // 判断玩家与敌人的距离是否在近战范围内
            // 这里使用飞刀环绕的半径作为近战范围，使其与飞刀动画一致
            double closeCombatRange = playerKnifeRadius + enemyKnifeRadius;
            
            // 当距离小于近战范围时，触发近战判定
            if (line.length() < closeCombatRange) {
                static QElapsedTimer lastCombatTimer;
                
                qDebug() << "近战判定 - 距离:" << line.length() 
                       << ", 玩家半径:" << playerKnifeRadius 
                       << ", 敌人半径:" << enemyKnifeRadius
                       << ", 总范围:" << closeCombatRange;
                
                // 限制近战判定频率为每秒最多5次 (200ms间隔)
                if (lastCombatTimer.isValid() && lastCombatTimer.elapsed() < 200) {
                    qDebug() << "近战判定 - 跳过，间隔太短:" << lastCombatTimer.elapsed() << "ms";
                    continue; // 时间间隔太短，跳过本次判定
                }
                
                // 重置计时器
                lastCombatTimer.start();
                
                // 近战攻击的抵消和伤害判定
                if (m_player->getKnives() > 0 && enemy->getKnives() > 0) {
                    // 玩家和敌人都有飞刀，触发抵消机制
                    qDebug() << "近战抵消 - 玩家飞刀: " << m_player->getKnives() 
                            << ", 敌人飞刀: " << enemy->getKnives()
                            << ", 距离: " << line.length() 
                            << ", 战斗范围: " << closeCombatRange;
                    
                    // 两者各减少一把飞刀
                    m_player->setKnives(m_player->getKnives() - 1);
                    enemy->setKnives(enemy->getKnives() - 1);
                    
                    // 产生抵消特效
                    QGraphicsEllipseItem* cancelEffect = new QGraphicsEllipseItem(-20, -20, 40, 40);
                    cancelEffect->setBrush(QBrush(QColor(255, 255, 0, 150))); // 半透明黄色
                    cancelEffect->setPen(QPen(Qt::yellow, 2));
                    cancelEffect->setPos((m_player->pos() + enemy->pos()) / 2); // 在两者中间位置
                    addItem(cancelEffect);
                    
                    // 200毫秒后移除特效
                    QTimer::singleShot(200, this, [this, cancelEffect]() {
                        if (cancelEffect->scene() == this) {
                            removeItem(cancelEffect);
                            delete cancelEffect;
                        }
                    });
                } 
                else if (m_player->getKnives() > 0 && enemy->getKnives() <= 0) {
                    // 玩家有飞刀，敌人没有飞刀，玩家对敌人造成伤害
                    qDebug() << "玩家近战攻击敌人 - 伤害成功, 距离: " << line.length() 
                            << ", 战斗范围: " << closeCombatRange;
                    
                    // 玩家消耗一把飞刀
                    m_player->setKnives(m_player->getKnives() - 1);
                    
                    // 敌人受到伤害
                    enemy->takeDamage(5);
                    
                    // 创建特效
                    QGraphicsEllipseItem* hitEffect = new QGraphicsEllipseItem(-25, -25, 50, 50);
                    hitEffect->setBrush(QBrush(QColor(255, 0, 0, 150))); // 半透明红色
                    hitEffect->setPen(QPen(Qt::red, 2));
                    hitEffect->setPos(enemy->pos());
                    addItem(hitEffect);
                    
                    // 200毫秒后移除特效
                    QTimer::singleShot(200, this, [this, hitEffect]() {
                        if (hitEffect->scene() == this) {
                            removeItem(hitEffect);
                            delete hitEffect;
                        }
                    });
                }
                else if (m_player->getKnives() <= 0 && enemy->getKnives() > 0) {
                    // 敌人有飞刀，玩家没有飞刀，敌人对玩家造成伤害
                    qDebug() << "敌人近战攻击玩家 - 伤害成功, 距离: " << line.length()
                            << ", 战斗范围: " << closeCombatRange;
                    
                    // 敌人消耗一把飞刀
                    enemy->setKnives(enemy->getKnives() - 1);
                    
                    // 玩家受到伤害
                    m_player->takeDamage(3);
                    
                    // 创建特效
                    QGraphicsEllipseItem* hitEffect = new QGraphicsEllipseItem(-25, -25, 50, 50);
                    hitEffect->setBrush(QBrush(QColor(255, 0, 0, 150))); // 半透明红色
                    hitEffect->setPen(QPen(Qt::red, 2));
                    hitEffect->setPos(m_player->pos());
                    addItem(hitEffect);
                    
                    // 200毫秒后移除特效
                    QTimer::singleShot(200, this, [this, hitEffect]() {
                        if (hitEffect->scene() == this) {
                            removeItem(hitEffect);
                            delete hitEffect;
                        }
                    });
                }
                // 如果双方都没有飞刀，不做任何操作
            }
        }
    }
    
    // 检测玩家与地图边界碰撞
    qreal playerDistance = QLineF(QPointF(0, 0), m_player->pos()).length();
    if (playerDistance > m_mapRadius - 40) {
        // 玩家超出边界，将其拉回边界内
        QPointF direction = m_player->pos() / playerDistance;
        m_player->setPos(direction * (m_mapRadius - 40));
    }
    
    // 检测敌人与地图边界碰撞
    for (Enemy* enemy : m_enemies) {
        if (enemy && enemy->isAlive()) {
            qreal enemyDistance = QLineF(QPointF(0, 0), enemy->pos()).length();
            if (enemyDistance > m_mapRadius - 30) {
                // 敌人超出边界，将其拉回边界内
                QPointF direction = enemy->pos() / enemyDistance;
                enemy->setPos(direction * (m_mapRadius - 30));
            }
        }
    }
}

void GameScene::spawnItems()
{
    if (!m_gameActive) return;
    
    // 随机生成道具
    int itemType = QRandomGenerator::global()->bounded(10);
    ItemType type;
    
    if (itemType < 7) {
        // 70%的概率生成飞刀
        type = ItemType::Knife;
    } else if (itemType < 9) {
        // 20%的概率生成治愈之心
        type = ItemType::Heart;
    } else {
        // 10%的概率生成疾步之靴
        type = ItemType::Boots;
    }
    
    // 创建道具并放置在随机位置
    Item* item = new Item(type);
    item->setPos(randomMapPosition());
    addItem(item);
    m_items.append(item);
}

void GameScene::handlePlayerDeath()
{
    if (!m_gameActive) return;
    
    qDebug() << "Player died. Game over sequence starting.";
    
    // 首先设置游戏状态为非活跃，防止重复触发
    m_gameActive = false;
    
    // 停止所有定时器，防止额外的更新产生冲突
    m_updateTimer.stop();
    m_collisionTimer.stop();
    m_itemSpawnTimer.stop();
    m_gameStatusTimer.stop();
    
    // 计算存活时间（秒）
    int surviveTime = m_gameTime.elapsed() / 1000;
    
    // 统计剩余敌人数量
    int aliveEnemies = 0;
    for (Enemy* enemy : m_enemies) {
        if (enemy && enemy->isAlive()) {
            aliveEnemies++;
        }
    }
    
    // 判断玩家是胜利还是失败
    bool victory = (aliveEnemies == 0);
    
    qDebug() << "Game over stats - Victory:" << victory << "Kills:" << m_kills << "Survive time:" << surviveTime;
    
    // 使用延时确保场景已经稳定后再发出游戏结束信号
    QTimer::singleShot(1000, this, [this, victory, surviveTime]() {
        qDebug() << "Emitting gameOver signal after delay";
        emit gameOver(victory, m_kills, surviveTime);
    });
}

void GameScene::checkGameStatus()
{
    if (!m_gameActive || !m_player) return;
    
    // 检查玩家状态
    if (!m_player->isAlive()) {
        qDebug() << "Player is not alive, triggering handlePlayerDeath";
        handlePlayerDeath();
        return;
    }
    
    // 检查敌人状态
    int aliveEnemies = 0;
    
    // 移除已经死亡并且不再在场景中的敌人
    for (int i = m_enemies.size() - 1; i >= 0; i--) {
        Enemy* enemy = m_enemies[i];
        if (!enemy) {
            // 无效的敌人引用，直接移除
            qDebug() << "Removing null enemy reference from enemies list at index" << i;
            m_enemies.removeAt(i);
            continue;
        }
        
        if (enemy->isAlive()) {
            aliveEnemies++;
        } else {
            // 敌人已死亡，确保从场景和列表中删除
            qDebug() << "Found dead enemy at index" << i;
            
            // 如果敌人仍在场景中，立即移除它
            if (enemy->scene() == this) {
                qDebug() << "Removing dead enemy from scene";
                removeItem(enemy);
            }
            
            // 从列表中移除敌人引用
            qDebug() << "Removing dead enemy from list";
            m_enemies.removeAt(i);
        }
    }
    
    // 如果所有敌人都死亡，玩家胜利
    if (aliveEnemies == 0) {
        qDebug() << "All enemies defeated, player wins";
        // 设置游戏为非活跃
        m_gameActive = false;
        
        // 停止所有定时器
        m_updateTimer.stop();
        m_collisionTimer.stop();
        m_itemSpawnTimer.stop();
        m_gameStatusTimer.stop();
        
        // 计算存活时间（秒）
        int surviveTime = m_gameTime.elapsed() / 1000;
        
        // 使用延时确保场景已经稳定后再发出胜利信号
        QTimer::singleShot(1000, this, [this, surviveTime]() {
            qDebug() << "Emitting victory gameOver signal after delay";
            emit gameOver(true, m_kills, surviveTime);
        });
    }
}

void GameScene::createMap()
{
    // 创建一个大的圆形背景
    QPixmap backgroundImage(":/images/background.jpg");
    
    if (backgroundImage.isNull()) {
        qDebug() << "Failed to load background image!";
        return;
    }
    
    // 创建一个2倍地图半径大小的背景
    QPixmap scaledBackground = backgroundImage.scaled(2 * m_mapRadius, 2 * m_mapRadius, 
                                                     Qt::KeepAspectRatioByExpanding, 
                                                     Qt::SmoothTransformation);
    
    // 使用遮罩创建圆形背景
    QPixmap circularBg(2 * m_mapRadius, 2 * m_mapRadius);
    circularBg.fill(Qt::transparent);
    
    QPainter painter(&circularBg);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    
    // 添加同心圆作为视觉引导
    painter.setPen(QPen(QColor(255, 255, 255, 80), 2));
    for (int radius = 200; radius <= m_mapRadius; radius += 200) {
        painter.drawEllipse(QPointF(m_mapRadius, m_mapRadius), radius, radius);
    }
    
    // 设置圆形裁剪区域并绘制背景
    painter.setClipRegion(QRegion(0, 0, 2 * m_mapRadius, 2 * m_mapRadius, QRegion::Ellipse));
    painter.drawPixmap(0, 0, scaledBackground);
    
    // 绘制边界
    painter.setPen(QPen(Qt::white, 5));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(m_mapRadius, m_mapRadius), m_mapRadius - 2.5, m_mapRadius - 2.5);
    
    // 添加背景到场景
    m_background = addPixmap(circularBg);
    m_background->setPos(-m_mapRadius, -m_mapRadius);
    
    // 创建地图边界（用于碰撞检测）
    m_mapBoundary = addEllipse(-m_mapRadius, -m_mapRadius, 2 * m_mapRadius, 2 * m_mapRadius, 
                              QPen(Qt::transparent), QBrush(Qt::transparent));
}

void GameScene::createBushes(int count)
{
    // 创建指定数量的树丛
    for (int i = 0; i < count; ++i) {
        // 创建随机大小的树丛
        int bushSize = QRandomGenerator::global()->bounded(80, 200);
        
        // 在地图上随机位置创建树丛
        QPointF bushPos = randomMapPosition();
        
        // 创建带有图形效果的树丛图形项
        QGraphicsEllipseItem* bush = new QGraphicsEllipseItem(-bushSize/2, -bushSize/2, bushSize, bushSize);
        
        // 使用半透明的绿色填充，增加真实感
        QRadialGradient gradient(QPointF(0, 0), bushSize/2);
        gradient.setColorAt(0, QColor(0, 120, 0, 220));
        gradient.setColorAt(0.8, QColor(0, 80, 0, 200));
        gradient.setColorAt(1, QColor(0, 60, 0, 180));
        
        bush->setBrush(QBrush(gradient));
        bush->setPen(QPen(QColor(0, 100, 0, 150), 1));
        
        // 设置Z值高于其他项，确保是最上层
        bush->setZValue(200);
        
        // 添加阴影效果
        QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
        shadowEffect->setColor(QColor(0, 0, 0, 100));
        shadowEffect->setOffset(3, 3);
        shadowEffect->setBlurRadius(10);
        bush->setGraphicsEffect(shadowEffect);
        
        // 将树丛添加到场景
        bush->setPos(bushPos);
        addItem(bush);
        
        // 添加到树丛列表
        m_bushes.append(bush);
    }
}

void GameScene::createPlayer()
{
    // 创建玩家角色
    m_player = new Player();
    m_player->setPos(0, 0);  // 初始位置在地图中心
    addItem(m_player);
    
    // 连接玩家死亡信号
    connect(m_player, &Player::playerDied, this, &GameScene::handlePlayerDeath);
}

void GameScene::createEnemies(int count)
{
    // 创建指定数量的敌人
    for (int i = 0; i < count; ++i) {
        Enemy* enemy = new Enemy();
        
        // 在地图上随机位置创建敌人，确保不会太靠近玩家
        QPointF pos;
        do {
            pos = randomMapPosition();
        } while (QLineF(pos, m_player->pos()).length() < 300);  // 至少距离玩家300个单位
        
        enemy->setPos(pos);
        
        // 确保敌人是可见的，重置所有状态
        enemy->setVisible(true);
        enemy->setOpacity(1.0);
        
        // 添加到场景
        addItem(enemy);
        
        // 连接敌人死亡信号
        connect(enemy, &Character::died, this, [this](Character* character) {
            m_kills++;
            qDebug() << "Enemy killed, total kills:" << m_kills;
        });
        
        // 添加到敌人列表
        m_enemies.append(enemy);
        
        qDebug() << "Created enemy at position:" << pos;
    }
}

void GameScene::createItems(int knifeCount, int heartCount, int bootsCount)
{
    // 创建飞刀道具
    for (int i = 0; i < knifeCount; ++i) {
        Item* knife = new Item(ItemType::Knife);
        knife->setPos(randomMapPosition());
        addItem(knife);
        m_items.append(knife);
    }
    
    // 创建治愈之心道具
    for (int i = 0; i < heartCount; ++i) {
        Item* heart = new Item(ItemType::Heart);
        heart->setPos(randomMapPosition());
        addItem(heart);
        m_items.append(heart);
    }
    
    // 创建疾步之靴道具
    for (int i = 0; i < bootsCount; ++i) {
        Item* boots = new Item(ItemType::Boots);
        boots->setPos(randomMapPosition());
        addItem(boots);
        m_items.append(boots);
    }
}

QPointF GameScene::randomMapPosition()
{
    // 在圆形地图内生成随机位置
    double radius = QRandomGenerator::global()->bounded(m_mapRadius * 0.9);
    double angle = QRandomGenerator::global()->bounded(2.0 * M_PI);
    
    double x = radius * qCos(angle);
    double y = radius * qSin(angle);
    
    return QPointF(x, y);
} 