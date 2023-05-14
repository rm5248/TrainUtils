/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_DATAGRAM_H
#define LIBLCC_DATAGRAM_H

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_datagram_buffer;

void lcc_datagram_append_bytes(struct lcc_datagram_buffer* datagram, void* bytes, int len);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
