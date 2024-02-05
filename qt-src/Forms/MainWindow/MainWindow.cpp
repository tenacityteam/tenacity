//
// Created by anerruption on 31/01/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainWindow.h" resolved

#include "MainWindow.h"
#include "Forms/ControlToolbar/ControlToolbar.h"
#include "ui_MainWindow.h"

#include <QDockWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    const auto controlToolbar{new QDockWidget("Control Toolbar", this)};
    controlToolbar->setWidget(new ControlToolbar(controlToolbar));

    addDockWidget(Qt::TopDockWidgetArea, controlToolbar);
}

MainWindow::~MainWindow() {
    delete ui;
}
