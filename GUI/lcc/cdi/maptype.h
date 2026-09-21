/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MAPTYPE_H
#define MAPTYPE_H

#include <QString>
#include <QVector>
#include <QVariant>
#include <variant>

class QXmlStreamReader;

class MapType
{
public:
    typedef std::variant<QString, int, uint64_t> Property;

    struct Relation {
        Property property;
        QString value;
        QString description;
    };

    MapType();

    static MapType createFromXML(QXmlStreamReader* xml);

    QString name() const;
    QString description() const;
    const QVector<Relation>& relations() const;

    /**
     * Convert a Property to a QVariant, for use as e.g. QComboBox item data.
     */
    static QVariant propertyToVariant(const Property& property);

private:
    QString m_name;
    QString m_description;
    QVector<Relation> m_relations;
};

#endif // MAPTYPE_H
