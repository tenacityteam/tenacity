//
// Created by anerruption on 02/07/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ToolsToolbar.h" resolved

#include "ToolsToolbar.h"
#include "ui_ToolsToolbar.h"

ToolsToolbar::ToolsToolbar(QWidget *parent) : QWidget(parent), ui(new Ui::ToolsToolbar) {
    ui->setupUi(this);
}

ToolsToolbar::~ToolsToolbar() {
    delete ui;
}
