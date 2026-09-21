/* SPDX-License-Identifier: GPL-2.0 */
#include <QStack>
#include <QXmlStreamReader>

#include "maptype.h"

MapType::MapType()
{

}

QString MapType::name() const{
    return m_name;
}

QString MapType::description() const{
    return m_description;
}

const QVector<MapType::Relation>& MapType::relations() const{
    return m_relations;
}

QVariant MapType::propertyToVariant(const Property& property){
    if(std::holds_alternative<QString>(property)){
        return QVariant(std::get<QString>(property));
    }else if(std::holds_alternative<int>(property)){
        return QVariant(std::get<int>(property));
    }else if(std::holds_alternative<uint64_t>(property)){
        return QVariant(static_cast<qulonglong>(std::get<uint64_t>(property)));
    }

    return QVariant();
}

MapType MapType::createFromXML(QXmlStreamReader* xml){
    MapType m;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "map"){
        // Invalid XML somehow??
        return m;
    }

    QStack<QString> tagStack;
    Relation currentRelation;

    while(!xml->atEnd()){
        type = xml->readNext();

        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "map" && tagStack.isEmpty()){
            break;
        }

        switch(type){
        case QXmlStreamReader::StartElement:
            tagStack.push(xml->name().toString());
            if(tagStack.top() == "relation"){
                currentRelation = Relation();
            }
            break;
        case QXmlStreamReader::EndElement:
            if(!tagStack.isEmpty()){
                if(tagStack.top() == "relation"){
                    m.m_relations.push_back(currentRelation);
                }
                tagStack.pop();
            }
            break;
        case QXmlStreamReader::Characters: {
            if(tagStack.isEmpty()){
                break;
            }
            bool inRelation = tagStack.contains(QStringLiteral("relation"));
            if(tagStack.top() == "name" && !inRelation){
                m.m_name = xml->text().toString();
            }else if(tagStack.top() == "description" && !inRelation){
                m.m_description = xml->text().toString();
            }else if(tagStack.top() == "property"){
                bool ok;
                int val = xml->text().toString().toInt(&ok);
                if(ok){
                    currentRelation.property = val;
                }else{
                    currentRelation.property = xml->text().toString();
                }
            }else if(tagStack.top() == "value"){
                currentRelation.value = xml->text().toString();
            }else if(tagStack.top() == "description" && inRelation){
                currentRelation.description = xml->text().toString();
            }
            break;
        }
        default:
            break;
        }

        if(xml->hasError()){
            break;
        }
    }

    return m;
}
