/* SPDX-License-Identifier: GPL-2.0 */
#include "identification.h"

#include <QStack>

Identification::Identification()
{

}

QString Identification::manufacturer(){
    return m_manufacturer;
}

QString Identification::model(){
    return m_model;
}

QString Identification::hardwareVersion(){
    return m_hardwareVersion;
}

QString Identification::softwareVersion(){
    return m_softwareVersion;
}

Identification Identification::createFromXML(QXmlStreamReader* xml){
    Identification i;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "identification"){
        // Invalid XML somehow??
        return i;
    }

    QStack<QString> tagStack;

    while(!xml->atEnd()){
        type = xml->readNext();

        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "identification" && tagStack.isEmpty()){
            break;
        }

        switch(type){
        case QXmlStreamReader::StartElement:
            tagStack.push(xml->name().toString());
            break;
        case QXmlStreamReader::EndElement:
            if(!tagStack.isEmpty()){
                tagStack.pop();
            }
            break;
        case QXmlStreamReader::Characters:
            if(!tagStack.isEmpty()){
                if(tagStack.top() == "manufacturer"){
                    i.m_manufacturer = xml->text().toString();
                }else if(tagStack.top() == "model"){
                    i.m_model = xml->text().toString();
                }else if(tagStack.top() == "hardwareVersion"){
                    i.m_hardwareVersion = xml->text().toString();
                }else if(tagStack.top() == "softwareVersion"){
                    i.m_softwareVersion = xml->text().toString();
                }
            }
            break;
        default:
            break;
        }

        if(xml->hasError()){
            break;
        }
    }

    return i;
}
