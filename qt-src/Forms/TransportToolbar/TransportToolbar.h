//
// Created by anerruption on 05/02/24.
//

#ifndef TRANSPORTTOOLBAR_H
#define TRANSPORTTOOLBAR_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class TransportToolbar; }
QT_END_NAMESPACE

class TransportToolbar final : public QWidget {
Q_OBJECT

public:
    explicit TransportToolbar(QWidget *parent = nullptr);
    ~TransportToolbar() override;

private:
    Ui::TransportToolbar *ui;
};

#endif //TRANSPORTTOOLBAR_H
