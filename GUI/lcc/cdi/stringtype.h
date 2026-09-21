/* SPDX-License-Identifier: GPL-2.0 */
#ifndef STRINGTYPE_H
#define STRINGTYPE_H

#include <QString>

class QXmlStreamReader;

class StringType
{
public:
    StringType();

    static StringType createFromXML(QXmlStreamReader* xml);

    QString name() const;
    QString description() const;
    int size() const;
    int offset() const;

private:
    QString m_name;
    QString m_description;
    int m_maxLen = 0;
    int m_offset = 0;
};

#endif // STRINGTYPE_H
