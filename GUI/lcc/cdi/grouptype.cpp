/* SPDX-License-Identifier: GPL-2.0 */
#include <QXmlStreamReader>
#include <QStack>
#include <QStringRef>

#include "grouptype.h"

GroupType::GroupType()
{

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

    QStack<QStringRef> tagStack;
    type = xml->readNext();

    while(!xml->atEnd()){
        QXmlStreamReader::TokenType type = xml->readNext();
        switch(type){
        case QXmlStreamReader::NoToken:
        case QXmlStreamReader::Invalid:
        case QXmlStreamReader::EndDocument:
        case QXmlStreamReader::StartDocument:
            break;
        case QXmlStreamReader::StartElement:
            tagStack.push(xml->name());
            if(tagStack.top() == "group"){
                CDIVariant v(std::make_shared<GroupType>(GroupType::createFromXML(xml)));
                g.m_elements.push_back(v);
            }else if(tagStack.top() == "int"){

            }
            break;
        case QXmlStreamReader::EndElement:
            tagStack.pop();
            break;
        case QXmlStreamReader::Characters:
            if(tagStack.size() > 0){
                if(tagStack.top() == "name"){
                    g.m_name = xml->text().toString();
                }else if(tagStack.top() == "description"){
                    g.m_description = xml->text().toString();
                }else if(tagStack.top() == "repname"){
                    g.m_repname = xml->text().toString();
                }
            }
        }

        if(xml->hasError()){
            break;
        }

        type = xml->readNext();
        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "group"){
            break;
        }
    }

    return g;
}
