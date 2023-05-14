/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include "lcc-datagram.h"
#include "lcc-common-internal.h"

void lcc_datagram_append_bytes(struct lcc_datagram_buffer* datagram, void* bytes, int len){
    if(len < 0){
        return;
    }

    if((datagram->offset + len) > sizeof(datagram->buffer)){
        // We don't have enough room to add in this data
        return;
    }

    memcpy(datagram->buffer + datagram->offset, bytes, len);
    datagram->offset += len;
}
