//
// Created by anerruption on 05/02/24.
//

#ifndef CONTROLTOOLBAR_H
#define CONTROLTOOLBAR_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class ControlToolbar; }
QT_END_NAMESPACE

class ControlToolbar final : public QWidget {
Q_OBJECT

public:
    explicit ControlToolbar(QWidget *parent = nullptr);
    ~ControlToolbar() override;

private:
    Ui::ControlToolbar *ui;
};

#endif //CONTROLTOOLBAR_H
