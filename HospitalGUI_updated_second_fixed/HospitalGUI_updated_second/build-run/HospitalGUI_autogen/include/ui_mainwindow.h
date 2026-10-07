/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *centralLayout;
    QStackedWidget *mainStackedWidget;
    QWidget *pageWelcome;
    QWidget *pageGateway;
    QWidget *pageLogin;
    QWidget *pageSignup;
    QWidget *pageEmergency;
    QWidget *pagePatient;
    QWidget *pageDoctor;
    QWidget *pageNurse;
    QWidget *pageManager;
    QWidget *pageInvoice;
    QWidget *pageDenied;
    QWidget *pagePending;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1280, 800);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        centralLayout = new QVBoxLayout(centralwidget);
        centralLayout->setObjectName("centralLayout");
        centralLayout->setContentsMargins(0, 0, 0, 0);
        mainStackedWidget = new QStackedWidget(centralwidget);
        mainStackedWidget->setObjectName("mainStackedWidget");
        pageWelcome = new QWidget();
        pageWelcome->setObjectName("pageWelcome");
        mainStackedWidget->addWidget(pageWelcome);
        pageGateway = new QWidget();
        pageGateway->setObjectName("pageGateway");
        mainStackedWidget->addWidget(pageGateway);
        pageLogin = new QWidget();
        pageLogin->setObjectName("pageLogin");
        mainStackedWidget->addWidget(pageLogin);
        pageSignup = new QWidget();
        pageSignup->setObjectName("pageSignup");
        mainStackedWidget->addWidget(pageSignup);
        pageEmergency = new QWidget();
        pageEmergency->setObjectName("pageEmergency");
        mainStackedWidget->addWidget(pageEmergency);
        pagePatient = new QWidget();
        pagePatient->setObjectName("pagePatient");
        mainStackedWidget->addWidget(pagePatient);
        pageDoctor = new QWidget();
        pageDoctor->setObjectName("pageDoctor");
        mainStackedWidget->addWidget(pageDoctor);
        pageNurse = new QWidget();
        pageNurse->setObjectName("pageNurse");
        mainStackedWidget->addWidget(pageNurse);
        pageManager = new QWidget();
        pageManager->setObjectName("pageManager");
        mainStackedWidget->addWidget(pageManager);
        pageInvoice = new QWidget();
        pageInvoice->setObjectName("pageInvoice");
        mainStackedWidget->addWidget(pageInvoice);
        pageDenied = new QWidget();
        pageDenied->setObjectName("pageDenied");
        mainStackedWidget->addWidget(pageDenied);
        pagePending = new QWidget();
        pagePending->setObjectName("pagePending");
        mainStackedWidget->addWidget(pagePending);

        centralLayout->addWidget(mainStackedWidget);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        mainStackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Health++", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
