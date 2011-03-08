#include "consolewindow.h"
#include "ui_consolewindow.h"
#include "qprocess.h"

ConsoleWindow::ConsoleWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ConsoleWindow)
{
    ui->setupUi(this);
}

ConsoleWindow::ConsoleWindow(QWidget *parent, const QString &exec_name, const QString &addr, const QString &port, const QString &bw, const QString &media_file, bool follower) :
    QMainWindow(parent),
    ui(new Ui::ConsoleWindow)
{
    ui->setupUi(this);
    qApp->installEventFilter(this);
    m_follower = follower;
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle("Console: " + exec_name);

    ui->textBrowser->append("Started " + exec_name + "!");
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clearLog()));
    connect(ui->killButton, SIGNAL(clicked()), this, SLOT(killProcess()));


    process = new QProcess();
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
    process->start(exec_name, QStringList() << addr << port << bw << media_file);

    process->waitForStarted();

    if(m_follower)
    {
      ui->textBrowser->append("Starting video...");
      vlc_proc = new QProcess();
      vlc_proc->start("vlc", QStringList() << "-I qt" << media_file);
      vlc_proc->waitForStarted();
    }
}

ConsoleWindow::~ConsoleWindow()
{
    if(m_follower)
      delete vlc_proc;
    delete process;
    delete ui;
}

bool ConsoleWindow::eventFilter(QObject* o, QEvent *e)
{
  if(e->type()  == QEvent::KeyPress)
  {
    QKeyEvent *ke = static_cast<QKeyEvent *>(e);
    if(ke->key() == Qt::Key_Escape)
    {
        this->close();
        return true;
    }
  }
  return QObject::eventFilter(o, e);
}

void ConsoleWindow::readOutput()
{
    if(process)
    {
        QByteArray result = process->readAllStandardOutput();
        QStringList lines = QString(result).split("\n");
        foreach (QString line, lines) {
            ui->textBrowser->append(line);
        }
    }
}

void ConsoleWindow::readError()
{
    if(process)
    {
        QByteArray result = process->readAllStandardError();
        QStringList lines = QString(result).split("\n");
        foreach (QString line, lines) {
            ui->textBrowser->append(line);
        }
    }
}

void ConsoleWindow::clearLog()
{
    ui->textBrowser->clear();
}

void ConsoleWindow::killProcess()
{
    process->close();
    this->close();
}
