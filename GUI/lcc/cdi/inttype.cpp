/* SPDX-License-Identifier: GPL-2.0 */
#include <QStack>
#include <QXmlStreamReader>
#include "inttype.h"

IntType::IntType()
{

}

QString IntType::name() const{
    return m_name;
}

QString IntType::description() const{
    return m_description;
}

std::optional<int> IntType::min() const{
    return m_min;
}

std::optional<int> IntType::max() const{
    return m_max;
}

std::optional<int> IntType::defaultValue() const{
    return m_default;
}

std::optional<MapType> IntType::map() const{
    return m_map;
}

int IntType::size() const{
    return m_storageSize;
}

int IntType::offset() const{
    return m_offset;
}

IntType IntType::createFromXML(QXmlStreamReader* xml){
    IntType i;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "int"){
        // Invalid XML somehow??
        return i;
    }

    QXmlStreamAttributes attrs = xml->attributes();
    if(attrs.hasAttribute("size")){
        i.m_storageSize = attrs.value("size").toInt();
    }
    if(attrs.hasAttribute("offset")){
        i.m_offset = attrs.value("offset").toInt();
    }

    QStack<QString> tagStack;

    while(!xml->atEnd()){
        type = xml->readNext();

        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "int" && tagStack.isEmpty()){
            break;
        }

        switch(type){
        case QXmlStreamReader::StartElement:
            if(xml->name() == "map"){
                i.m_map = MapType::createFromXML(xml);
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
                    i.m_name = xml->text().toString();
                }else if(tagStack.top() == "description"){
                    i.m_description = xml->text().toString();
                }else if(tagStack.top() == "min"){
                    bool ok;
                    int val = xml->text().toString().toInt(&ok);
                    if(ok){
                        i.m_min = val;
                    }
                }else if(tagStack.top() == "max"){
                    bool ok;
                    int val = xml->text().toString().toInt(&ok);
                    if(ok){
                        i.m_max = val;
                    }
                }else if(tagStack.top() == "default"){
                    bool ok;
                    int val = xml->text().toString().toInt(&ok);
                    if(ok){
                        i.m_default = val;
                    }
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
