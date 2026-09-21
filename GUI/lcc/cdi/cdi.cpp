/* SPDX-License-Identifier: GPL-2.0 */
#include "cdi.h"

CDI::CDI()
{

}

Identification CDI::identification(){
    return m_ident;
}

ACDI CDI::acdi(){
    return m_acdi;
}

const QVector<Segment>& CDI::segments() const{
    return m_segments;
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
            }else if(xml->name() == "acdi"){
                c.m_acdi = ACDI::createFromXML(xml);
            }else if(xml->name() == "segment"){
                c.m_segments.push_back(Segment::createFromXML(xml));
            }
            break;
        default:
            break;
        }

        if(xml->hasError()){
            break;
        }
    }

    return c;
}
