#ifndef CONNECTABLE_H
#define CONNECTABLE_H

#include <QVector>
#include <QPoint>

class Connectable
{
public:
    Connectable();
    virtual ~Connectable();

    virtual QVector<QPoint> connectionPoints() = 0;
};

#endif // CONNECTABLE_H
