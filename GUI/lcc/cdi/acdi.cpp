/* SPDX-License-Identifier: GPL-2.0 */
#include <QXmlStreamReader>

#include "acdi.h"

ACDI::ACDI()
{

}

ACDI ACDI::createFromXML(QXmlStreamReader* xml){
    ACDI a;

    QXmlStreamReader::TokenType type = xml->tokenType();
    if(type != QXmlStreamReader::StartElement ||
            xml->name() != "acdi"){
        // Invalid XML somehow??
        return a;
    }

    while(!xml->atEnd()){
        type = xml->readNext();

        if(type == QXmlStreamReader::EndElement &&
                xml->name() == "acdi"){
            break;
        }

        if(xml->hasError()){
            break;
        }
    }

    return a;
}
