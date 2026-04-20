#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include "gameview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startGame();
    void exitGame();
    void returnToMainMenu();

private:
    // 创建主菜单
    void createMainMenu();
    
    // 游戏视图
    GameView* m_gameView;
    
    // 主菜单组件
    QWidget* m_centralWidget;
    QVBoxLayout* m_layout;
    QLabel* m_titleLabel;
    QPushButton* m_startButton;
    QPushButton* m_exitButton;
};
#endif // MAINWINDOW_H
