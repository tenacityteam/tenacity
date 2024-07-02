//
// Created by anerruption on 05/02/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ControlToolbar.h" resolved

#include "TransportToolbar.h"
#include "ui_TransportToolbar.h"

TransportToolbar::TransportToolbar(QWidget *parent) : QWidget(parent), ui(new Ui::TransportToolbar) {
    ui->setupUi(this);
}

TransportToolbar::~TransportToolbar() {
    delete ui;
}
