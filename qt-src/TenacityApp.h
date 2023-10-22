/******************************************************************************

  Tenacity

  TenacityApp.h

  Avery King

  License: GPL v2 or later

*******************************************************************************/

#include <QApplication>


class TenacityApp final : public QApplication
{
    public:
        TenacityApp(int& argc, char** argv);
};
