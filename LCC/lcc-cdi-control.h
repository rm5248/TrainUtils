/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_CDI_CONTROL_H
#define LCC_CDI_CONTROL_H

#include <stdint.h>

struct lcc_simple_node_info;

#define LCC_CDI_INPUT_TYPE_NONE 0
#define LCC_CDI_INPUT_TYPE_INT 1
#define LCC_CDI_INPUT_TYPE_EVENTID 2
#define LCC_CDI_INPUT_TYPE_STRING 3

typedef void (*lcc_cdi_xml_data)(const char* str, int len, void* cb_data);

struct lcc_cdi_relation{
    int32_t property;
    const char* value;
};

struct lcc_cdi_input{
    const char* name;
    const char* description;
    struct lcc_cdi_relation* relations;
    int32_t min;
    int32_t max;
    uint8_t input_type;
    uint8_t size;
    uint8_t offset;
};

struct lcc_cdi_group{
    const char* name;
    const char* repname;
    uint8_t replication;
    uint8_t offset;
    /** NULL terminated list of inputs */
    struct lcc_cdi_input* inputs;
};

struct lcc_cdi_segment{
    uint8_t segment_space;
    const char* segment_name;
    /** NULL terminated list of groups */
    struct lcc_cdi_group* groups;
    /** NULL terminated list of inputs */
    struct lcc_cdi_input* inputs;
};

struct lcc_cdi_control{
    struct lcc_cdi_segment* segments;
    struct lcc_simple_node_info* simple_info;
};

/**
 * Convert the given CDI data to XML.
 * Note: this method is not thread-safe.
 *
 * @param cdi_control The CDI struct to convert to XML
 * @param xml_cb A callback that will be called with XML data in 64 byte chunks.
 * @return
 */
int lcc_cdi_control_to_xml(struct lcc_cdi_control* cdi_control, lcc_cdi_xml_data xml_cb, void* cb_data);

/**
 * Determine the length of the CDI.  Returns the XML length, or a negative errcode.
 *
 * @param cdi_control
 * @return
 */
int lcc_cdi_control_xml_length(struct lcc_cdi_control* cdi_control);

#endif // LCC_CDI_CONTROL_H
