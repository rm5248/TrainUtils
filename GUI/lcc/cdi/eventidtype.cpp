/* SPDX-License-Identifier: GPL-2.0 */
#include <QStack>
#include <QXmlStreamReader>

#include "eventidtype.h"

EventIDType::EventIDType()
{

}

QString EventIDType::name() const{
    return m_name;
}

QString EventIDType::description() const{
    return m_description;
}

int EventIDType::offset() const{
    return m_offset;
}

EventIDType EventIDType::createFromXML(QXmlStreamReader* xml){
    EventIDType e;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "eventid"){
        // Invalid XML somehow??
        return e;
    }

    QXmlStreamAttributes attrs = xml->attributes();
    if(attrs.hasAttribute("offset")){
        e.m_offset = attrs.value("offset").toInt();
    }

    QStack<QString> tagStack;

    while(!xml->atEnd()){
        type = xml->readNext();

        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "eventid" && tagStack.isEmpty()){
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
                if(tagStack.top() == "name"){
                    e.m_name = xml->text().toString();
                }else if(tagStack.top() == "description"){
                    e.m_description = xml->text().toString();
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

    return e;
}
