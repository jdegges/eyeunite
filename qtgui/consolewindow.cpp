#include "consolewindow.h"
#include "ui_consolewindow.h"
#include "qprocess.h"

ConsoleWindow::ConsoleWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ConsoleWindow)
{
    ui->setupUi(this);
}

ConsoleWindow::ConsoleWindow(QWidget *parent, const QString &exec_name, const QStringList &args) :
    QMainWindow(parent),
    ui(new Ui::ConsoleWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle("Console: " + exec_name);

    ui->textBrowser->append("Started " + exec_name + "!");
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clearLog()));
    connect(ui->killButton, SIGNAL(clicked()), this, SLOT(killProcess()));

    process = new QProcess();
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
    process->start(exec_name, args);
}

ConsoleWindow::~ConsoleWindow()
{
    delete process;
    delete ui;
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
