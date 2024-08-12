#ifndef LIBLCC_CONFIG_H
#define LIBLCC_CONFIG_H

#cmakedefine LIBLCC_DEBUG
#cmakedefine LIBLCC_ENABLE_STATIC_CONTEXT

#ifdef LIBLCC_ENABLE_STATIC_CONTEXT
#ifndef LIBLCC_EVENT_LIST_STATIC_SIZE
#define LIBLCC_EVENT_LIST_STATIC_SIZE ${LIBLCC_EVENT_LIST_STATIC_SIZE}
#endif
#endif

#endif
