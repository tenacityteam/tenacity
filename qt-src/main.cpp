//
// Created by anerruption on 03/02/24.
//

#include "Forms/MainWindow/MainWindow.h"

#include <QApplication>

int main(int argc, char** argv)
{
    QCoreApplication::setApplicationName("Tenacity");

    QApplication a{argc, argv};

    MainWindow mainWindow{};
    mainWindow.show();

    return QApplication::exec();
}