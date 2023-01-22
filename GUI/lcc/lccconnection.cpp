/* SPDX-License-Identifier: GPL-2.0 */
#include "lccconnection.h"

LCCConnection::LCCConnection(QObject *parent) : QObject(parent)
{
    m_lcc = lcc_context_new();

    // TODO make this configurable.  currently set to 'assigned by software at runtime'
    lcc_context_set_unique_identifer(m_lcc, 0x040000000001);
}

LCCConnection::~LCCConnection(){
    lcc_context_free(m_lcc);
}

void LCCConnection::setName(QString name){
    m_name = name;
}

QString LCCConnection::name() const{
    return m_name;
}

void LCCConnection::setSimpleNodeInformation(QString manufacturer,
                              QString model,
                              QString hwVersion,
                              QString swVersion){
    std::string manufStd = manufacturer.toStdString();
    std::string modelStd = model.toStdString();
    std::string hwStd = hwVersion.toStdString();
    std::string swStd = swVersion.toStdString();

    lcc_context_set_simple_node_information(m_lcc,
                                            manufStd.c_str(),
                                            modelStd.c_str(),
                                            hwStd.c_str(),
                                            swStd.c_str());
}

void LCCConnection::setSimpleNodeNameDescription(QString nodeName,
                                  QString nodeDescription){
    std::string nameStd = nodeName.toStdString();
    std::string descriptionStd = nodeDescription.toStdString();

    lcc_context_set_simple_node_name_description(m_lcc,
                                                 nameStd.c_str(),
                                                 descriptionStd.c_str());
}
