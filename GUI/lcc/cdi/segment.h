/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SEGMENT_H
#define SEGMENT_H

#include <string>
#include <QString>

/**
 * Represents a <segment> tag in the LCC XML schema.
 */
class Segment
{
public:
    Segment();

private:
    int m_space;
    int m_origin;
    QString m_name;
    QString m_description;
};

#endif // SEGMENT_H
