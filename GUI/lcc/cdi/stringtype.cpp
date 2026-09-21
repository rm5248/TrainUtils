/* SPDX-License-Identifier: GPL-2.0 */
#include <QStack>
#include <QXmlStreamReader>

#include "stringtype.h"

StringType::StringType()
{

}

QString StringType::name() const{
    return m_name;
}

QString StringType::description() const{
    return m_description;
}

int StringType::size() const{
    return m_maxLen;
}

int StringType::offset() const{
    return m_offset;
}

StringType StringType::createFromXML(QXmlStreamReader* xml){
    StringType s;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "string"){
        // Invalid XML somehow??
        return s;
    }

    QXmlStreamAttributes attrs = xml->attributes();
    if(attrs.hasAttribute("size")){
        s.m_maxLen = attrs.value("size").toInt();
    }
    if(attrs.hasAttribute("offset")){
        s.m_offset = attrs.value("offset").toInt();
    }

    QStack<QString> tagStack;

    while(!xml->atEnd()){
        type = xml->readNext();

        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "string" && tagStack.isEmpty()){
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
