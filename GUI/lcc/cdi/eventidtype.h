/* SPDX-License-Identifier: GPL-2.0 */
#ifndef EVENTIDTYPE_H
#define EVENTIDTYPE_H

#include <QString>

class QXmlStreamReader;

class EventIDType
{
public:
    EventIDType();

    static EventIDType createFromXML(QXmlStreamReader* xml);

    QString name() const;
    QString description() const;
    int offset() const;

private:
    QString m_name;
    QString m_description;
    int m_offset = 0;
};

#endif // EVENTIDTYPE_H
