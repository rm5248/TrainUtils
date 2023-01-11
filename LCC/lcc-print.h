/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_PRINT_H
#define LIBLCC_PRINT_H

#include <stdio.h>

#include "lcc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decode an LCC frame, printing out useful information
 *
 * @param frame The frame to print
 * @param output The file to output to
 * @param print_flags Controls output format
 */
void lcc_decode_frame(struct lcc_can_frame* frame, FILE* output, int print_flags);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
