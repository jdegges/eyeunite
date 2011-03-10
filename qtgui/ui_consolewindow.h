/********************************************************************************
** Form generated from reading UI file 'consolewindow.ui'
**
** Created: Thu Mar 10 00:32:33 2011
**      by: Qt User Interface Compiler version 4.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONSOLEWINDOW_H
#define UI_CONSOLEWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QStatusBar>
#include <QtGui/QTextBrowser>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ConsoleWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTextBrowser *textBrowser;
    QHBoxLayout *horizontalLayout;
    QPushButton *clearButton;
    QPushButton *killButton;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *ConsoleWindow)
    {
        if (ConsoleWindow->objectName().isEmpty())
            ConsoleWindow->setObjectName(QString::fromUtf8("ConsoleWindow"));
        ConsoleWindow->resize(394, 307);
        centralwidget = new QWidget(ConsoleWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        textBrowser = new QTextBrowser(centralwidget);
        textBrowser->setObjectName(QString::fromUtf8("textBrowser"));

        verticalLayout->addWidget(textBrowser);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        clearButton = new QPushButton(centralwidget);
        clearButton->setObjectName(QString::fromUtf8("clearButton"));
        clearButton->setMaximumSize(QSize(100, 25));

        horizontalLayout->addWidget(clearButton);

        killButton = new QPushButton(centralwidget);
        killButton->setObjectName(QString::fromUtf8("killButton"));
        killButton->setMaximumSize(QSize(100, 25));

        horizontalLayout->addWidget(killButton);


        verticalLayout->addLayout(horizontalLayout);

        ConsoleWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(ConsoleWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 394, 27));
        ConsoleWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(ConsoleWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        ConsoleWindow->setStatusBar(statusbar);

        retranslateUi(ConsoleWindow);

        QMetaObject::connectSlotsByName(ConsoleWindow);
    } // setupUi

    void retranslateUi(QMainWindow *ConsoleWindow)
    {
        ConsoleWindow->setWindowTitle(QApplication::translate("ConsoleWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        clearButton->setText(QApplication::translate("ConsoleWindow", "Clear", 0, QApplication::UnicodeUTF8));
        killButton->setText(QApplication::translate("ConsoleWindow", "Kill", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class ConsoleWindow: public Ui_ConsoleWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONSOLEWINDOW_H
