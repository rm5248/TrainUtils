/* SPDX-License-Identifier: GPL-2.0 */
#ifndef EVENTGRAPHSTORAGE_H
#define EVENTGRAPHSTORAGE_H

#include <QMap>
#include <QPointF>
#include <QString>

#include "eventgraphmodel.h"

/**
 * What is saved for the event graph of a connection: where the user put the
 * nodes, and the pin directions that have been learned.  The links themselves
 * are not saved, as they come from the nodes.
 */
struct EventGraphLayout {
    /** Canvas position of each node, by node ID */
    QMap<uint64_t, QPointF> positions;
    QMap<QString, QMap<QString, PinDirection>> learnedDirections;
};

/**
 * JSON save/load of an EventGraphLayout, one file per connection under
 * <app config>/eventgraphs/.
 */
namespace EventGraphStorage {

bool save(const EventGraphLayout& layout, const QString& connectionName);

/** Returns an empty layout if nothing has been saved for the connection */
EventGraphLayout load(const QString& connectionName);

QByteArray toJson(const EventGraphLayout& layout);
EventGraphLayout fromJson(const QByteArray& json);

}

#endif // EVENTGRAPHSTORAGE_H
