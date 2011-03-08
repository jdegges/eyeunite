#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "consolewindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    exec_name = "./eyeunitesource";

    qApp->installEventFilter(this);
    ui->addr->setFocus();

    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(spawnProcess()));
    connect(ui->sourceButton, SIGNAL(clicked()), this, SLOT(setExec_Source()));
    connect(ui->followerButton, SIGNAL(clicked()), this, SLOT(setExec_Follower()));

    eyeuniteIcon = new QIcon("./statusicon.png");
    setWindowIcon(*eyeuniteIcon);


    trayIcon = new QSystemTrayIcon(*eyeuniteIcon);
    trayIcon->show();
    trayIcon->setIcon(*eyeuniteIcon);
}

MainWindow::~MainWindow()
{
    if(eyeuniteIcon)
      delete eyeuniteIcon;
    if(trayIcon)
      delete trayIcon;
    if(ui)
      delete ui;
}

bool MainWindow::eventFilter(QObject* o, QEvent *e)
{
  if(e->type()  == QEvent::KeyPress)
  {
    QKeyEvent *ke = static_cast<QKeyEvent *>(e);
    if(ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)
    {
      ui->pushButton->click();
      return true;
    }
    else if(ke->key() == Qt::Key_Escape)
    {
        this->close();
        return true;
    }
  }
  return QObject::eventFilter(o, e);
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
    ConsoleWindow* cw = new ConsoleWindow(0, exec_name, ui->addr->text(), ui->port->text(), ui->bw->text(), ui->media_file->text(), ui->followerButton->isChecked());
    cw->show();
}
