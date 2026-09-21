/* SPDX-License-Identifier: GPL-2.0 */
#ifndef GROUPTYPE_H
#define GROUPTYPE_H

#include <QString>
#include <QVector>
#include <memory>
#include <variant>

class QXmlStreamReader;

class GroupType;
class IntType;
class StringType;
class EventIDType;

// Because a GroupType can have multiple Groups, we do the (somewhat ugly)
// solution of making everything a shared_ptr for consistency
typedef std::variant<std::shared_ptr<IntType>,
    std::shared_ptr<StringType>,
    std::shared_ptr<EventIDType>,
    std::shared_ptr<GroupType>> CDIVariant;

class GroupType
{
public:
    GroupType();

    static GroupType createFromXML(QXmlStreamReader* xml);

    QString name() const;
    QString description() const;
    QString repname() const;
    int replication() const;
    int offset() const;
    const QVector<CDIVariant>& elements() const;

private:
    QString m_name;
    QString m_description;
    QString m_repname;
    QVector<CDIVariant> m_elements;
    int m_offset = 0;
    int m_replication = 0;
};

#endif // GROUPTYPE_H
