#ifndef CABBUS_TO_LOCONET_H
#define CABBUS_TO_LOCONET_H

#include "CabBus.h"
#include "loconet_buffer.h"

typedef void (*cabbus_read_fn)(void*);
typedef void (*loconet_read_fn)(void*);

void cabbus_to_loconet_main( struct cabbus_context* cab_context,
                             cab_write_fn cab_write,
                             cabbus_read_fn cab_read,
                             void* cab_read_fn_data,
                             struct loconet_context* loconet_context,
                             loconet_read_fn loconet_read,
                             void* loconet_read_fn_data);

#endif
