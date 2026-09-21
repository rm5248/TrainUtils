/* SPDX-License-Identifier: GPL-2.0 */
#include <QXmlStreamReader>
#include <QStack>

#include "grouptype.h"
#include "inttype.h"
#include "stringtype.h"
#include "eventidtype.h"

GroupType::GroupType()
{

}

QString GroupType::name() const{
    return m_name;
}

QString GroupType::description() const{
    return m_description;
}

QString GroupType::repname() const{
    return m_repname;
}

int GroupType::replication() const{
    return m_replication;
}

int GroupType::offset() const{
    return m_offset;
}

const QVector<CDIVariant>& GroupType::elements() const{
    return m_elements;
}

GroupType GroupType::createFromXML(QXmlStreamReader* xml){
    GroupType g;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "group"){
        // Invalid XML somehow??
        return g;
    }

    QXmlStreamAttributes attrs = xml->attributes();
    if(attrs.hasAttribute("replication")){
        g.m_replication = attrs.value("replication").toInt();
    }
    if(attrs.hasAttribute("offset")){
        g.m_offset = attrs.value("offset").toInt();
    }

    QStack<QString> tagStack;

    while(!xml->atEnd()){
        type = xml->readNext();

        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "group" && tagStack.isEmpty()){
            break;
        }

        switch(type){
        case QXmlStreamReader::StartElement:
            if(xml->name() == "group"){
                g.m_elements.push_back(CDIVariant(std::make_shared<GroupType>(GroupType::createFromXML(xml))));
            }else if(xml->name() == "int"){
                g.m_elements.push_back(CDIVariant(std::make_shared<IntType>(IntType::createFromXML(xml))));
            }else if(xml->name() == "string"){
                g.m_elements.push_back(CDIVariant(std::make_shared<StringType>(StringType::createFromXML(xml))));
            }else if(xml->name() == "eventid"){
                g.m_elements.push_back(CDIVariant(std::make_shared<EventIDType>(EventIDType::createFromXML(xml))));
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
                    g.m_name = xml->text().toString();
                }else if(tagStack.top() == "description"){
                    g.m_description = xml->text().toString();
                }else if(tagStack.top() == "repname"){
                    g.m_repname = xml->text().toString();
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

    return g;
}
