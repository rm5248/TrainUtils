/* SPDX-License-Identifier: GPL-2.0 */
#include <QXmlStreamReader>
#include <QStack>
#include <QStringRef>

#include "segment.h"
#include "grouptype.h"

Segment::Segment()
{

}

Segment Segment::createFromXML(QXmlStreamReader* xml){
    Segment s;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "segment"){
        // Invalid XML somehow??
        return s;
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

            }else if(tagStack.top() == "string"){

            }else if(tagStack.top() == "int"){

            }else if(tagStack.top() == "eventid"){

            }
            break;
        case QXmlStreamReader::EndElement:
            tagStack.pop();
            break;
        case QXmlStreamReader::Characters:
            if(tagStack.size() > 0){
                if(tagStack.top() == "name"){
                    s.m_name = xml->text().toString();
                }else if(tagStack.top() == "description"){
                    s.m_description = xml->text().toString();
                }
            }
        }

        if(xml->hasError()){
            break;
        }

        type = xml->readNext();
        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "segment"){
            break;
        }
    }

    return s;
}
