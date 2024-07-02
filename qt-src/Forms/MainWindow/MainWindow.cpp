//
// Created by anerruption on 31/01/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainWindow.h" resolved

#include "MainWindow.h"
#include "Forms/ToolsToolbar/ToolsToolbar.h"
#include "Forms/TransportToolbar/TransportToolbar.h"
#include "ui_MainWindow.h"

#include <QDockWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    const auto transportToolbar{new QDockWidget("Transport Toolbar", this)};
    const auto toolsToolbar{new QDockWidget("Tools Toolbar", this)};

    transportToolbar->setWidget(new TransportToolbar(transportToolbar));
    toolsToolbar->setWidget(new ToolsToolbar(toolsToolbar));

    addDockWidget(Qt::TopDockWidgetArea, transportToolbar);
    addDockWidget(Qt::TopDockWidgetArea, toolsToolbar);

    ui->actionTransport->setChecked(true);
    ui->actionTools->setChecked(true);
}

MainWindow::~MainWindow() {
    delete ui;
}