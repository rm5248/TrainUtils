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
