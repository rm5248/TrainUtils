#include "speedometerdisplay.h"
#include "ui_speedometerdisplay.h"

SpeedometerDisplay::SpeedometerDisplay(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SpeedometerDisplay)
{
    ui->setupUi(this);
}

SpeedometerDisplay::~SpeedometerDisplay()
{
    delete ui;
}

void SpeedometerDisplay::incomingSpeedMeasurement(double speed){
    if(ui->mphRadio->isChecked()){
        speed = speed / 1.609;
    }

    ui->speedLabel->setText(QString("%1").arg(speed));
}
