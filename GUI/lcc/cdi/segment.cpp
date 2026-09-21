/* SPDX-License-Identifier: GPL-2.0 */
#include <QXmlStreamReader>
#include <QStack>

#include "segment.h"
#include "grouptype.h"
#include "inttype.h"
#include "stringtype.h"
#include "eventidtype.h"

Segment::Segment()
{

}

QString Segment::name() const{
    return m_name;
}

QString Segment::description() const{
    return m_description;
}

int Segment::space() const{
    return m_space;
}

int Segment::origin() const{
    return m_origin;
}

const QVector<CDIVariant>& Segment::elements() const{
    return m_elements;
}

Segment Segment::createFromXML(QXmlStreamReader* xml){
    Segment s;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "segment"){
        // Invalid XML somehow??
        return s;
    }

    QXmlStreamAttributes attrs = xml->attributes();
    if(attrs.hasAttribute("space")){
        s.m_space = attrs.value("space").toInt();
    }
    if(attrs.hasAttribute("origin")){
        s.m_origin = attrs.value("origin").toInt();
    }

    QStack<QString> tagStack;

    while(!xml->atEnd()){
        type = xml->readNext();

        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "segment" && tagStack.isEmpty()){
            break;
        }

        switch(type){
        case QXmlStreamReader::StartElement:
            if(xml->name() == "group"){
                s.m_elements.push_back(CDIVariant(std::make_shared<GroupType>(GroupType::createFromXML(xml))));
            }else if(xml->name() == "int"){
                s.m_elements.push_back(CDIVariant(std::make_shared<IntType>(IntType::createFromXML(xml))));
            }else if(xml->name() == "string"){
                s.m_elements.push_back(CDIVariant(std::make_shared<StringType>(StringType::createFromXML(xml))));
            }else if(xml->name() == "eventid"){
                s.m_elements.push_back(CDIVariant(std::make_shared<EventIDType>(EventIDType::createFromXML(xml))));
            }else{
                tagStack.push(xml->name().toString());
            }
            break;
        case QXmlStreamReader::EndElement:
            if(!tagStack.isEmpty()){
                tagStack.pop();
            }
            break;
        case QXmlStreamReader::Characters:
            if(!tagStack.isEmpty()){
                if(tagStack.top() == "name"){
                    s.m_name = xml->text().toString();
                }else if(tagStack.top() == "description"){
                    s.m_description = xml->text().toString();
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

    return s;
}
