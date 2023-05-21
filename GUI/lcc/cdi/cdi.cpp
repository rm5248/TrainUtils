/* SPDX-License-Identifier: GPL-2.0 */
#include "cdi.h"

CDI::CDI()
{

}

Identification CDI::identification(){
    return m_ident;
}

CDI CDI::createFromXML(QXmlStreamReader* xml){
    CDI c;

    while(!xml->atEnd()){
        QXmlStreamReader::TokenType type = xml->readNext();
        switch(type){
        case QXmlStreamReader::NoToken:
        case QXmlStreamReader::Invalid:
        case QXmlStreamReader::EndDocument:
            break;
        case QXmlStreamReader::StartDocument:
            // Good, we should be here?
            break;
        case QXmlStreamReader::StartElement:
            if(xml->name() == "identification"){
                c.m_ident = Identification::createFromXML(xml);
            }
            break;
        }

        if(xml->hasError()){
            break;
        }
    }

    return c;
}
