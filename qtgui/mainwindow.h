#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QKeyEvent>
#include <QEvent>
#include <QSystemTrayIcon>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void spawnProcess();
    void setExec_Source();
    void setExec_Follower();

protected:
    bool eventFilter(QObject *o, QEvent *e);

private:
    Ui::MainWindow *ui;
    QString exec_name;
    QIcon *eyeuniteIcon;
    QSystemTrayIcon *trayIcon;
};

#endif // MAINWINDOW_H
