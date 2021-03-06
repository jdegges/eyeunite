#ifndef CONSOLEWINDOW_H
#define CONSOLEWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QEvent>
#include <QKeyEvent>

namespace Ui {
    class ConsoleWindow;
}

class ConsoleWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ConsoleWindow(QWidget *parent = 0);
    explicit ConsoleWindow(QWidget *parent = 0, const QString &exec_name = "", const QString &addr = "", const QString &port = "", const QString &bw = "", const QString &media_file = "", bool follower = false);
    ~ConsoleWindow();

public slots:
    void readOutput();
    void readError();
    void clearLog();
    void killProcess();

protected:
    bool eventFilter(QObject *o, QEvent *e);

private:
    Ui::ConsoleWindow *ui;
    QProcess* process;
    QProcess* vlc_proc;
    bool m_follower;
    QString m_media_file;
};

#endif // CONSOLEWINDOW_H
