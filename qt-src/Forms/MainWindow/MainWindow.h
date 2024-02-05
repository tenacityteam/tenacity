//
// Created by anerruption on 31/01/24.
//

#ifndef TENACITY_MAINWINDOW_H
#define TENACITY_MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
};

#endif//TENACITY_MAINWINDOW_H
