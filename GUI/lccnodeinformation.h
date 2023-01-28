/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNODEINFORMATION_H
#define LCCNODEINFORMATION_H

#include <QWidget>

namespace Ui {
class LCCNodeInformation;
}

class LCCNodeInformation : public QWidget
{
    Q_OBJECT

public:
    explicit LCCNodeInformation(QWidget *parent = nullptr);
    ~LCCNodeInformation();

private:
    Ui::LCCNodeInformation *ui;
};

#endif // LCCNODEINFORMATION_H
