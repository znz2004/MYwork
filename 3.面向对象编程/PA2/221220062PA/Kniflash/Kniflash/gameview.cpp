#include "gameview.h"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPushButton>
#include <QGraphicsProxyWidget>
#include <QGraphicsDropShadowEffect>
#include <QLinearGradient>
#include <QBrush>
#include <QPen>

GameView::GameView(QWidget *parent)
    : QGraphicsView(parent),
      m_gameScene(nullptr)
{
    // 设置视图属性
    setRenderHint(QPainter::Antialiasing);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // 创建游戏场景
    m_gameScene = new GameScene(this);
    setScene(m_gameScene);
    
    // 连接游戏结束信号
    connect(m_gameScene, &GameScene::gameOver, this, &GameView::handleGameOver);
    
    // 设置大小
    resize(1080, 675);
    
    // 设置定时器更新视图（使视图跟随玩家）
    QTimer* viewTimer = new QTimer(this);
    connect(viewTimer, &QTimer::timeout, this, &GameView::updateView);
    viewTimer->start(16);  // 约60FPS
    
    // 设置视图可以获取焦点
    setFocusPolicy(Qt::StrongFocus);
    
    qDebug() << "GameView initialized";
}

GameView::~GameView()
{
    qDebug() << "GameView destructor called";
}

void GameView::startGame()
{
    qDebug() << "Starting new game";
    
    // 确保使用游戏场景而不是游戏结束场景
    setScene(m_gameScene);
    
    // 重置变换
    resetTransform();
    
    // 初始化游戏
    if (m_gameScene) {
        m_gameScene->resetGame(); // 使用resetGame而不是initGame来确保完全重置
    } else {
        qDebug() << "Error: Game scene is null!";
        return;
    }
    
    // 显示视图
    show();
    
    // 设置焦点
    setFocus();
}

void GameView::handleGameOver(bool victory, int kills, int surviveTime)
{
    qDebug() << "handleGameOver called - Victory:" << victory << "Kills:" << kills << "Survive time:" << surviveTime;
    
    // 确保在主事件循环中执行
    QMetaObject::invokeMethod(this, "showGameOverScreen", Qt::QueuedConnection,
                             Q_ARG(bool, victory),
                             Q_ARG(int, kills),
                             Q_ARG(int, surviveTime));
}

void GameView::showGameOverScreen(bool victory, int kills, int surviveTime)
{
    qDebug() << "Showing game over screen";
    
    // 创建游戏结束场景
    QGraphicsScene* gameOverScene = new QGraphicsScene(this);
    
    // 设置场景背景
    QLinearGradient gradient(0, 0, 0, height());
    if (victory) {
        gradient.setColorAt(0, QColor(0, 100, 0)); // 深绿色
        gradient.setColorAt(1, QColor(0, 50, 0));
    } else {
        gradient.setColorAt(0, QColor(100, 0, 0)); // 深红色
        gradient.setColorAt(1, QColor(50, 0, 0));
    }
    
    gameOverScene->setSceneRect(0, 0, width(), height());
    
    // 创建背景矩形
    QGraphicsRectItem* bgRect = new QGraphicsRectItem(0, 0, width(), height());
    bgRect->setBrush(QBrush(gradient));
    gameOverScene->addItem(bgRect);
    
    // 添加标题文本
    QFont titleFont("Arial", 40, QFont::Bold);
    QGraphicsTextItem* titleItem = gameOverScene->addText(
        victory ? "胜利!" : "失败!", 
        titleFont
    );
    titleItem->setDefaultTextColor(Qt::white);
    titleItem->setPos((width() - titleItem->boundingRect().width()) / 2, 80);
    
    // 添加闪光效果到标题
    QGraphicsDropShadowEffect* glowEffect = new QGraphicsDropShadowEffect();
    glowEffect->setColor(victory ? Qt::green : Qt::red);
    glowEffect->setOffset(0);
    glowEffect->setBlurRadius(15);
    titleItem->setGraphicsEffect(glowEffect);
    
    // 创建数据面板背景
    QGraphicsRectItem* statsPanelBg = new QGraphicsRectItem(width()/4, 180, width()/2, 200);
    statsPanelBg->setBrush(QBrush(QColor(0, 0, 0, 150))); // 半透明黑色
    statsPanelBg->setPen(QPen(Qt::white, 2));
    gameOverScene->addItem(statsPanelBg);
    
    // 添加游戏统计文本
    QFont statsFont("Arial", 18);
    
    // 击杀数
    QGraphicsTextItem* killsText = gameOverScene->addText(
        QString("击杀数: %1").arg(kills), 
        statsFont
    );
    killsText->setDefaultTextColor(Qt::white);
    killsText->setPos(width()/4 + 20, 200);
    
    // 存活时间
    QGraphicsTextItem* timeText = gameOverScene->addText(
        QString("存活时间: %1 秒").arg(surviveTime), 
        statsFont
    );
    timeText->setDefaultTextColor(Qt::white);
    timeText->setPos(width()/4 + 20, 240);
    
    // 玩家排名（如果失败）
    if (!victory) {
        QGraphicsTextItem* rankText = gameOverScene->addText(
            QString("最终排名: #%1/6").arg(6 - kills), 
            statsFont
        );
        rankText->setDefaultTextColor(Qt::white);
        rankText->setPos(width()/4 + 20, 280);
    }
    
    // 创建返回按钮
    QPushButton* returnButton = new QPushButton("返回主菜单");
    returnButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  padding: 10px 20px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #3e8e41;"
        "}"
    );
    
    // 添加按钮到场景
    QGraphicsProxyWidget* buttonProxy = gameOverScene->addWidget(returnButton);
    buttonProxy->setPos((width() - buttonProxy->boundingRect().width()) / 2, 400);
    
    // 连接按钮点击信号
    connect(returnButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Return button clicked, returning to main menu";
        hide();
        emit gameExited();
    });
    
    // 设置场景
    setScene(gameOverScene);
    
    // 重置变换使整个场景可见
    resetTransform();
    
    // 显示游戏结束场景
    show();
}

void GameView::keyPressEvent(QKeyEvent *event)
{
    // 将按键事件转发给场景
    if (m_gameScene && m_gameScene->getPlayer()) {
        m_gameScene->getPlayer()->keyPressEvent(event);
    } else {
        // 如果没有玩家对象，继续默认处理
        QGraphicsView::keyPressEvent(event);
    }
    
    // ESC键退出游戏
    if (event->key() == Qt::Key_Escape) {
        qDebug() << "ESC key pressed, exiting game";
        hide();
        emit gameExited();
    }
}

void GameView::keyReleaseEvent(QKeyEvent *event)
{
    // 将按键事件转发给场景
    if (m_gameScene && m_gameScene->getPlayer()) {
        m_gameScene->getPlayer()->keyReleaseEvent(event);
    } else {
        // 如果没有玩家对象，继续默认处理
        QGraphicsView::keyReleaseEvent(event);
    }
}

void GameView::updateView()
{
    // 如果玩家存在，视图中心跟随玩家
    if (m_gameScene && m_gameScene->getPlayer()) {
        centerOn(m_gameScene->getPlayer());
    }
} 