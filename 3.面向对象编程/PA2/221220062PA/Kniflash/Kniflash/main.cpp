#include "mainwindow.h"

#include <QApplication>
#include <QSplashScreen>
#include <QTimer>
#include <QPainter>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 创建自定义启动画面
    QPixmap splashPixmap(800, 450);
    splashPixmap.fill(Qt::black);
    
    // 在启动画面上添加游戏名称
    QPainter painter(&splashPixmap);
    painter.setPen(Qt::white);
    QFont titleFont("Arial", 40, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(splashPixmap.rect(), Qt::AlignCenter, "Kniflash");
    
    QSplashScreen splash(splashPixmap);
    splash.show();
    
    // 2秒后显示主窗口
    MainWindow w;
    QTimer::singleShot(2000, &splash, &QSplashScreen::close);
    QTimer::singleShot(2000, &w, &MainWindow::show);
    
    return a.exec();
}
