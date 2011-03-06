#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "consolewindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose, true);

    exec_name = "./eyeunitesource";

    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(spawnProcess()));
    connect(ui->sourceButton, SIGNAL(clicked()), this, SLOT(setExec_Source()));
    connect(ui->followerButton, SIGNAL(clicked()), this, SLOT(setExec_Follower()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setExec_Follower()
{
    exec_name = "./eyeunitefollower";
    ui->label->setText("Lobby Token");
}

void MainWindow::setExec_Source()
{
    exec_name = "./eyeunitesource";
    ui->label->setText("IP Address");
}

void MainWindow::spawnProcess()
{
    QStringList args;
    ConsoleWindow* cw = new ConsoleWindow(0, exec_name, (args << ui->addr->toPlainText() << ui->port->toPlainText() << ui->bw->toPlainText() << ui->media_file->toPlainText()));
    cw->show();
}
