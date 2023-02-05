/* SPDX-License-Identifier: GPL-2.0 */
#include "throttledisplay.h"
#include "ui_throttledisplay.h"
#include "throttle.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.ThrottleDisplay" );

ThrottleDisplay::ThrottleDisplay(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ThrottleDisplay)
{
    ui->setupUi(this);
}

ThrottleDisplay::~ThrottleDisplay()
{
    delete ui;
}

void ThrottleDisplay::setThrottle(std::shared_ptr<Throttle> throttle){
    m_throttle = throttle;
}

void ThrottleDisplay::on_forwardRadio_clicked()
{
    configureFwdRev();
}


void ThrottleDisplay::on_reverseRadio_clicked()
{
    configureFwdRev();
}

void ThrottleDisplay::configureFwdRev(){
    if(ui->forwardRadio->isChecked()){
        m_throttle->setDirection(LocomotiveDirection::Forward);
    }else{
        m_throttle->setDirection(LocomotiveDirection::Reverse);
    }
}

void ThrottleDisplay::on_locoSpeed_valueChanged(int value)
{
    m_throttle->setSpeed(value);
    ui->speedLabel->setText(QString("%1 %").arg(value));
}

void ThrottleDisplay::on_selectLoco_clicked()
{
    bool ok;
    int number = ui->locoNumber->text().toInt(&ok);
    if(!ok){
        LOG4CXX_ERROR(logger, "Loco number not valid");
        return;
    }
    m_throttle->selectLocomotive(number, true);
}


void ThrottleDisplay::on_func0_clicked()
{
    m_throttle->toggleFunction(0);
}

