#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QProcess"

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

private:
    Ui::MainWindow *ui;
    QString exec_name;
};

#endif // MAINWINDOW_H
