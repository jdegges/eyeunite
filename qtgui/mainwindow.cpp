#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "consolewindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label->hide();
    ui->addr->hide();

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

    QString addr, port, bw, media_file;

    config_file = new QFile(".config");
    config_file->open(QIODevice::ReadWrite | QIODevice::Text);
    QTextStream in(config_file);
    in >> addr >> port >> bw >> media_file;
    ui->addr->setText(addr);
    ui->port->setText(port);
    ui->bw->setText(bw);
    ui->media_file->setText(media_file);
}

MainWindow::~MainWindow()
{
    if(config_file)
    {
      config_file->remove();
      config_file->open(QIODevice::ReadWrite | QIODevice::Text);
      QTextStream out(config_file);
      out << ui->addr->text() << "\n" << ui->port->text() << "\n" << ui->bw->text() << "\n" << ui->media_file->text() << "\n";
      config_file->close();
      delete config_file;
    }
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
    ui->label->show();
    ui->addr->show();
}

void MainWindow::setExec_Source()
{
    exec_name = "./eyeunitesource";
    ui->label->setText("IP Address");
    ui->label->hide();
    ui->addr->hide();
}

void MainWindow::spawnProcess()
{
    QStringList args;
    ConsoleWindow* cw = new ConsoleWindow(0, exec_name, ui->addr->text(), ui->port->text(), ui->bw->text(), ui->media_file->text(), ui->followerButton->isChecked());
    cw->show();
}
