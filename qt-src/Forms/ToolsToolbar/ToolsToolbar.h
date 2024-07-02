//
// Created by anerruption on 02/07/24.
//

#ifndef TOOLSTOOLBAR_H
#define TOOLSTOOLBAR_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class ToolsToolbar; }
QT_END_NAMESPACE

class ToolsToolbar : public QWidget {
Q_OBJECT

public:
    explicit ToolsToolbar(QWidget *parent = nullptr);
    ~ToolsToolbar() override;

private:
    Ui::ToolsToolbar *ui;
};


#endif //TOOLSTOOLBAR_H
