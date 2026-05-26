#include "speedoconnection.h"

SpeedoConnection::SpeedoConnection(QObject *parent)
    : SystemConnection{parent}
{}

std::shared_ptr<Turnout> SpeedoConnection::getDCCTurnout(int){
    return std::shared_ptr<Turnout>();
}

void SpeedoConnection::load(QSettings& settings){

}

void SpeedoConnection::doSave(QSettings& settings){

}

QString SpeedoConnection::connectionType(){
    return "speedo";
}

bool SpeedoConnection::open(){

}
