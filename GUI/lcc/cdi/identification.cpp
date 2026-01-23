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

    QStack<QStringView> tagStack;
    type = xml->readNext();

    while(!xml->atEnd()){

        switch(type){
        case QXmlStreamReader::NoToken:
        case QXmlStreamReader::Invalid:
        case QXmlStreamReader::EndDocument:
            break;
        case QXmlStreamReader::StartElement:
            tagStack.push(xml->name());
            break;
        case QXmlStreamReader::EndElement:
            tagStack.pop();
            break;
        case QXmlStreamReader::Characters:
            if(tagStack.size() > 0){
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
        }

        if(xml->hasError()){
            break;
        }

        type = xml->readNext();
        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "identification"){
            break;
        }
    }

    return i;
}
