/******************************************************************************

  Tenacity

  TenacityApp.cpp

  Avery King

  License: GPL v2 or later

*******************************************************************************/

#include "TenacityApp.h"
#include <QMainWindow>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QMainWindow frame;
    app.setApplicationName("QTenacity");

    frame.show();
    return app.exec();
}
