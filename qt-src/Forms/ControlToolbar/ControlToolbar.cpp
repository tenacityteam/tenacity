//
// Created by anerruption on 05/02/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ControlToolbar.h" resolved

#include "ControlToolbar.h"
#include "ui_ControlToolbar.h"

ControlToolbar::ControlToolbar(QWidget *parent) : QWidget(parent), ui(new Ui::ControlToolbar) {
    ui->setupUi(this);

    ui->pause->setIcon(QIcon::fromTheme("media-playback-pause"));
    ui->play->setIcon(QIcon::fromTheme("media-playback-start"));
    ui->stop->setIcon(QIcon::fromTheme("media-playback-stop"));
    ui->loop->setIcon(QIcon::fromTheme("media-playlist-repeat"));
    ui->skipToStart->setIcon(QIcon::fromTheme("media-skip-backward"));
    ui->skipToEnd->setIcon(QIcon::fromTheme("media-skip-forward"));
    ui->record->setIcon(QIcon::fromTheme("media-record"));
}

ControlToolbar::~ControlToolbar() {
    delete ui;
}
