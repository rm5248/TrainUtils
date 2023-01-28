/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnodeinformation.h"
#include "ui_lccnodeinformation.h"

LCCNodeInformation::LCCNodeInformation(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCCNodeInformation)
{
    ui->setupUi(this);

    std::vector<QCheckBox*> checkboxes = {
        ui->reservation,
        ui->teachingLearning,
        ui->firmwareUpgrade,
        ui->functionConfiguration,
        ui->display,
        ui->remoteButton,
        ui->datagram,
        ui->functionDescription,
        ui->simpleProtocol,
        ui->producerConsumer,
        ui->simpleNodeInfo,
        ui->firmwareUpgradeActive,
        ui->tractionControl,
        ui->memoryConfig,
        ui->identification,
        ui->abbreviatedCDI,
        ui->commandStation,
        ui->stream,
        ui->simpleTrainNode,
        ui->CDI,
    };

    for(QCheckBox* checkbox : checkboxes){
        checkbox->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
}

LCCNodeInformation::~LCCNodeInformation()
{
    delete ui;
}
