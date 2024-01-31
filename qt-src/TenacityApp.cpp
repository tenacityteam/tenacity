/******************************************************************************

  Tenacity

  TenacityApp.cpp

  Avery King

  License: GPL v2 or later

*******************************************************************************/

#include "TenacityApp.h"
#include "Forms/MainWindow/MainWindow.h"

int main(int argc, char** argv)
{
    QCoreApplication::setApplicationName("Tenacity");

    QApplication a(argc, argv);

    MainWindow mainWindow {};
    mainWindow.show();

    return QApplication::exec();
}
