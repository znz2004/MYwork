#include "mainwindow.h"
#include <QFontDatabase>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_gameView(nullptr),
      m_centralWidget(nullptr),
      m_layout(nullptr),
      m_titleLabel(nullptr),
      m_startButton(nullptr),
      m_exitButton(nullptr)
{
    // 设置窗口标题和大小
    setWindowTitle("Kniflash");
    resize(1080, 675);
    
    // 创建主菜单
    createMainMenu();
    
    // 创建游戏视图
    m_gameView = new GameView();
    
    // 连接游戏退出信号
    connect(m_gameView, &GameView::gameExited, this, &MainWindow::returnToMainMenu);
    
    // 居中显示窗口
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    qDebug() << "MainWindow initialized";
}

MainWindow::~MainWindow()
{
    delete m_gameView;
}

void MainWindow::startGame()
{
    // 隐藏主窗口
    hide();
    
    // 启动游戏
    m_gameView->startGame();
}

void MainWindow::exitGame()
{
    // 退出应用
    close();
}

void MainWindow::returnToMainMenu()
{
    qDebug() << "Returning to main menu";
    
    // 显示主窗口
    show();
    
    // 激活窗口
    activateWindow();
    raise();
    
    // 为下一次游戏做准备
    if (m_gameView) {
        m_gameView->hide();
    }
}

void MainWindow::createMainMenu()
{
    // 创建中央控件
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    // 创建布局
    m_layout = new QVBoxLayout(m_centralWidget);
    m_layout->setAlignment(Qt::AlignCenter);
    m_layout->setSpacing(30);
    
    // 创建标题
    m_titleLabel = new QLabel("Kniflash", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(48);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    // 创建按钮
    m_startButton = new QPushButton("开始游戏", this);
    m_exitButton = new QPushButton("退出游戏", this);
    
    // 设置按钮样式
    QFont buttonFont;
    buttonFont.setPointSize(16);
    
    m_startButton->setFont(buttonFont);
    m_exitButton->setFont(buttonFont);
    
    m_startButton->setMinimumSize(200, 50);
    m_exitButton->setMinimumSize(200, 50);
    
    // 连接按钮信号
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startGame);
    connect(m_exitButton, &QPushButton::clicked, this, &MainWindow::exitGame);
    
    // 添加组件到布局
    m_layout->addWidget(m_titleLabel);
    m_layout->addWidget(m_startButton);
    m_layout->addWidget(m_exitButton);
    
    // 设置样式
    setStyleSheet(
        "QLabel { color: #333333; }"
        "QPushButton { background-color: #4CAF50; color: white; border-radius: 5px; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3e8e41; }"
        "QMainWindow { background-color: #f0f0f0; }"
    );
}
