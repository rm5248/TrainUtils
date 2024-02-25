/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_TEST_COMMON_H
#define LCC_TEST_COMMON_H

struct lcc_context;

/**
 * Create a number of LCC contexts
 *
 * @param num Number of contexts to create and put on the network
 *
 * @return Array of contexts
 */
struct lcc_context** lcctest_create_contexts(int num);

/**
 * Free the lcc_contexts created by lcctest_create_context.
 */
void lcctest_free_contexts();

#endif
