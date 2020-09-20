﻿#include "mainwindow.h"
#include "ui_mainwindow.h"
#define MAXNUM 100
#include <QPainter>
#include <QMouseEvent>
#include <math.h>
#include <QMessageBox>
#include<QDebug>
#include <QTimer>
#include<QUdpSocket>

#include"GameModel.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
     , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ser=new QTcpServer;
    ser->listen(QHostAddress::AnyIPv4,8888);
    connect(ser,&QTcpServer::newConnection,this,&MainWindow::newCon);
    //setMouseTracking(true);

    // 设置棋盘大小
    setFixedSize(M * 2 + BLOCK_SIZE * GRADSIZE,
                 M * 2 + BLOCK_SIZE * GRADSIZE);

    // 开始游戏
    inGame();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    // 绘制棋盘
    painter.setRenderHint(QPainter::Antialiasing, true); // 抗锯齿

    for (int i = 0; i < GRADSIZE + 1; i++)
    {
        // 从左到右，第(i+1)条竖线
        painter.drawLine(M + BLOCK_SIZE * i, M, M + BLOCK_SIZE * i, size().height() - M);
        // 从上到下，第（i+1)条横线
        painter.drawLine(M, M + BLOCK_SIZE * i, size().width() - M, M + BLOCK_SIZE * i);
    }

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    // 绘制落子标记(防止鼠标出框越界)
    if (Rowclick > 0 && Rowclick < GRADSIZE &&
        Colclick > 0 && Colclick < GRADSIZE &&
        game->mapGV[Rowclick][Colclick] == 0)
    {
        if (game->player)
            brush.setColor(Qt::black);
        else
            brush.setColor(Qt::white);
        painter.setBrush(brush);
        painter.drawRect(M + BLOCK_SIZE * Colclick - MSIZE / 2, M + BLOCK_SIZE * Rowclick - MSIZE / 2, MSIZE, MSIZE);
    }

    // 绘制棋子
    for (int i = 0; i < GRADSIZE; i++)
        for (int j = 0; j < GRADSIZE; j++)
        {
            if (game->mapGV[i][j] == 1)//黑棋
            {
                //brush.setColor(Qt::white);
                brush.setColor(Qt::black);

                painter.setBrush(brush);
                painter.drawEllipse(M + BLOCK_SIZE * j - CHESSR, M + BLOCK_SIZE * i - CHESSR, CHESSR * 2, CHESSR * 2);
            }
            else if (game->mapGV[i][j] == -1)
            {//白棋
                //brush.setColor(Qt::black);
                brush.setColor(Qt::white);

                painter.setBrush(brush);
                painter.drawEllipse(M + BLOCK_SIZE * j - CHESSR, M + BLOCK_SIZE * i - CHESSR, CHESSR * 2, CHESSR * 2);
            }
        }

    // 判断输赢
    if (Rowclick > 0 && Rowclick < GRADSIZE &&
        Colclick > 0 && Colclick < GRADSIZE &&
        (game->mapGV[Rowclick][Colclick] == 1 ||
            game->mapGV[Rowclick][Colclick] == -1))
    {
        if (game->winorlose(Rowclick, Colclick) && game->gameStatus == PLAYING)
        {
            game->gameStatus = WIN;
            QString str;
            if (game->mapGV[Rowclick][Colclick] == 1)
                str = "黑棋";
            else if (game->mapGV[Rowclick][Colclick] == -1)
                str = "白棋";
            QMessageBox::StandardButton btnValue = QMessageBox::information(this, "六子棋", str + " 胜利!");

            // 重置游戏状态，否则容易死循环
            if (btnValue == QMessageBox::Ok)
            {
                game->start(game_type);
                game->gameStatus = PLAYING;
            }
        }
        if(game->isFour(Rowclick, Colclick) && game->gameStatus == PLAYING&&game->mapGV[Rowclick][Colclick] == 1){
            game->gameStatus=WIN;
            QString str="白棋";
            QMessageBox::StandardButton btnValue = QMessageBox::information(this, "六子棋", str + "胜利!");

            if (btnValue == QMessageBox::Ok)
            {
                game->start(game_type);
                game->gameStatus = PLAYING;
            }
        }
        if(game->isFive(Rowclick, Colclick) && game->gameStatus == PLAYING){
            game->gameStatus=WIN;
            QString str="白棋";
            QMessageBox::StandardButton btnValue = QMessageBox::information(this, "六子棋", str + "胜利!");

            if (btnValue == QMessageBox::Ok)
            {
                game->start(game_type);
                game->gameStatus = PLAYING;
            }
        }
    }
    if (game->isDeadGame())
    {
        QMessageBox::StandardButton btnValue = QMessageBox::information(this, "oops", "和棋!");
        if (btnValue == QMessageBox::Ok)
        {
            game->start(game_type);
            game->gameStatus = PLAYING;
        }

    }

}
void MainWindow::newCon()
{
    QTcpSocket *socket=ser->nextPendingConnection(); //创建socket连接

    QHostAddress clientAddress =socket->peerAddress();
    quint16 clientPort=socket->peerPort();

    ui->addressLineEdit->setText(clientAddress.toString());
    ui->portlineEdit->setText(QString::number(clientPort));

    connect(socket,&QTcpSocket::readyRead,this,&MainWindow::dealData);
}

void MainWindow::dealData()
{
    QTcpSocket *socket=(QTcpSocket *)sender(); //获取信号的发出者

    ui->textEdit->setText(QString(socket->readAll()));
}
void MainWindow::inGame()
{
    // 初始化游戏模型
    game = new GameModel;
    inAIGame();
}

void MainWindow::inAIGame()
{
    game_type = AI;
    game->gameStatus = PLAYING;
    game->start(game_type);
    update();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 通过鼠标的hover确定落子的标记
    int x = event->x();
    int y = event->y();

    // 棋盘边缘不能落子
    if (x >= M + BLOCK_SIZE / 2 &&
            x < size().width() - M - BLOCK_SIZE / 2  &&
            y >= M + BLOCK_SIZE / 2 &&
            y < size().height()- M - BLOCK_SIZE / 2)
    {
        // 获取最近的左上角的点
        // add by rock
        int col = (x - M) / BLOCK_SIZE;
        int row = (y - M) / BLOCK_SIZE;


        int leftX = M + BLOCK_SIZE * col;
        int leftY = M + BLOCK_SIZE * row;

        // 根据距离算出合适的点击位置,一共四个点，根据半径距离选最近的
        Rowclick = -1; // 初始化最终的值
        Colclick = -1;
        int len = 0; // 计算完后取整就可以了

        sPos = false;

        // 确定一个误差在范围内的点，且只可能确定一个出来
        len = sqrt((x - leftX) * (x - leftX) + (y - leftY) * (y - leftY));
        if (len < OFFSET)
        {
            Rowclick = row;
            Colclick = col;
            if (game->mapGV[Rowclick][Colclick]==0) {
                sPos = true;
            }
        }

        len = sqrt((x - leftX - BLOCK_SIZE) * (x - leftX - BLOCK_SIZE) + (y - leftY) * (y - leftY));
        if (len < OFFSET)
        {
            Rowclick = row;
            Colclick = col + 1;
            if (game->mapGV[Rowclick][Colclick]==0) {
                sPos = true;
            }
        }

        len = sqrt((x - leftX) * (x - leftX) + (y - leftY - BLOCK_SIZE) * (y - leftY - BLOCK_SIZE));
        if (len < OFFSET)
        {
            Rowclick = row + 1;
            Colclick = col;
            if (game->mapGV[Rowclick][Colclick]==0) {
                sPos = true;
            }
        }

        len = sqrt((x - leftX - BLOCK_SIZE) * (x - leftX - BLOCK_SIZE) + (y - leftY - BLOCK_SIZE) * (y - leftY - BLOCK_SIZE));
        if (len < OFFSET)
        {
            Rowclick = row + 1;
            Colclick = col + 1;

            if (game->mapGV[Rowclick][Colclick]==0) {
                sPos = true;
            }

        }
    }



    // 存了坐标后也要重绘
    update();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *)
{
    if (sPos == false) {
        return;
    } else {
        sPos = false;
    }

    if(game_type==PERSON){//人人模式
        chessPerson();
    }

    else if (game_type == AI) { //人机模式
        // AI 下棋
        chessPerson();
        QTimer::singleShot(AITHITIME, this, SLOT(chessAI()));
    }
}



void MainWindow::chessPerson()
{
    // 根据当前存储的坐标下子
    // 只有有效点击才下子，并且该处没有子
    if (Rowclick != -1 && Colclick != -1 && game->mapGV[Rowclick][Colclick] == 0)
    {

        game->actionMan(Rowclick, Colclick);
        // 重绘
        update();
    }
}

void MainWindow::chessAI()
{
    if(game_type==AI){
        game->actionAI(Rowclick, Colclick);
        update();
    }
}


void MainWindow::on_btn_PVP_clicked()
{
    game_type=PERSON;
    game->start(game_type);
    game->gameStatus = PLAYING;
}

void MainWindow::on_btn_PVC_clicked()
{
    game_type=AI;
    game->start(game_type);
    game->gameStatus = PLAYING;
}

void MainWindow::on_btn_CVC_clicked()
{
    game->actionAI(Rowclick, Colclick);
    update();
}
