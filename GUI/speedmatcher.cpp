#include "speedmatcher.h"
#include "ui_speedmatcher.h"

SpeedMatcher::SpeedMatcher(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SpeedMatcher)
{
    ui->setupUi(this);
}

SpeedMatcher::~SpeedMatcher()
{
    delete ui;
}
