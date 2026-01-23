/* SPDX-License-Identifier: GPL-2.0 */
#include <QStack>
#include <QXmlStreamReader>
#include "inttype.h"

IntType::IntType()
{

}

IntType IntType::createFromXML(QXmlStreamReader* xml){
    IntType i;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "int"){
        // Invalid XML somehow??
        return i;
    }

    QStack<QStringView> tagStack;
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
            if(tagStack.top() == "map"){
                i.m_map = MapType::createFromXML(xml);
            }
            break;
        case QXmlStreamReader::EndElement:
            tagStack.pop();
            break;
        case QXmlStreamReader::Characters:
            if(tagStack.size() > 0){
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
                        i.m_max = val;
                    }
                }
            }
        }

        if(xml->hasError()){
            break;
        }

        type = xml->readNext();
        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "int"){
            break;
        }
    }

    return i;
}
