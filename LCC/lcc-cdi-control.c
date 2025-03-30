/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>

#include "lcc-cdi-control.h"
#include "lcc-common.h"
#include "lcc-common-internal.h"

static char cdi_buffer[64];
static int cdi_buffer_loc = 0;
static char tmp_buffer[64];

static void lcc_cdi_xml_data_count(const char*, int len, void* cb_data){
    int* i = cb_data;
    *i += len;
}

static void append_char(char c, lcc_cdi_xml_data xml_cb, void* cb_data){
    cdi_buffer[cdi_buffer_loc] = c;
    cdi_buffer_loc++;

    if(cdi_buffer_loc >= sizeof(cdi_buffer)){
        xml_cb(cdi_buffer, cdi_buffer_loc, cb_data);
        cdi_buffer_loc = 0;
    }
}

static void append_string(const char* c, lcc_cdi_xml_data xml_cb, void* cb_data){
    for(int x = 0; x < strlen(c); x++){
        append_char(c[x], xml_cb, cb_data);
    }
}


static void output_input(struct lcc_cdi_input* input, lcc_cdi_xml_data xml_cb, void* cb_data){
    const char* tag = NULL;
    switch(input->input_type){
    case LCC_CDI_INPUT_TYPE_INT:
        tag = "int"; break;
    case LCC_CDI_INPUT_TYPE_EVENTID:
        tag = "eventid"; break;
    case LCC_CDI_INPUT_TYPE_STRING:
        tag = "string"; break;
    default:
        return;
    }

    if(input->offset != 0 && input->size != 0){
        snprintf(tmp_buffer, sizeof(tmp_buffer), "<%s offset='%d' size='%d'>",
                 tag,
                 input->offset,
                 input->size);
        append_string(tmp_buffer, xml_cb, cb_data);
    }else if(input->size != 0){
        snprintf(tmp_buffer, sizeof(tmp_buffer), "<%s size='%d'>",
                 tag,
                 input->size);
        append_string(tmp_buffer, xml_cb, cb_data);
    }else{
        append_string("<", xml_cb, cb_data);
        append_string(tag, xml_cb, cb_data);
        append_string(">", xml_cb, cb_data);
    }

    append_string("<name>", xml_cb, cb_data);
    append_string(input->name, xml_cb, cb_data);
    append_string("</name>", xml_cb, cb_data);

    append_string("<description>", xml_cb, cb_data);
    append_string(input->description, xml_cb, cb_data);
    append_string("</description>", xml_cb, cb_data);

    if(input->min != 0){
        snprintf(tmp_buffer, sizeof(tmp_buffer), "<min>%d</min>",
                 input->min);
        append_string(tmp_buffer, xml_cb, cb_data);
    }

    if(input->max != 0){
        snprintf(tmp_buffer, sizeof(tmp_buffer), "<max>%d</max>",
                 input->max);
        append_string(tmp_buffer, xml_cb, cb_data);
    }


    struct lcc_cdi_relation* relation = input->relations;
    if(relation != NULL){
        append_string("<map>", xml_cb, cb_data);
        while(relation != NULL){
            append_string("<relation>", xml_cb, cb_data);
            append_string("<property>", xml_cb, cb_data);
            snprintf(tmp_buffer, sizeof(tmp_buffer), "%d",
                     relation->property);
            append_string(tmp_buffer, xml_cb, cb_data);
            append_string("</property>", xml_cb, cb_data);
            append_string("<value>", xml_cb, cb_data);
            snprintf(tmp_buffer, sizeof(tmp_buffer), "%s",
                     relation->value);
            append_string(tmp_buffer, xml_cb, cb_data);
            append_string("</value>", xml_cb, cb_data);
            append_string("</relation>", xml_cb, cb_data);
            relation++;
        }
        append_string("</map>", xml_cb, cb_data);
    }

    append_string("</", xml_cb, cb_data);
    append_string(tag, xml_cb, cb_data);
    append_string(">", xml_cb, cb_data);
}

static void output_group(struct lcc_cdi_group* group, lcc_cdi_xml_data xml_cb, void* cb_data){
    if(group->offset != 0 || group->replication != 0){
        snprintf(tmp_buffer, sizeof(tmp_buffer), "<group offset='%d' replication='%d'>", group->offset, group->replication);
        append_string(tmp_buffer, xml_cb, cb_data);
    }else{
        append_string("<group>", xml_cb, cb_data);
    }

    append_string("<name>", xml_cb, cb_data);
    append_string(group->name, xml_cb, cb_data);
    append_string("</name>", xml_cb, cb_data);
    if(group->repname != NULL){
        append_string("<repname>", xml_cb, cb_data);
        append_string(group->repname, xml_cb, cb_data);
        append_string("</repname>", xml_cb, cb_data);
    }

    struct lcc_cdi_input* input = group->inputs;
    while(input != NULL){
        output_input(input, xml_cb, cb_data);
        input++;
    }

    append_string("</group>", xml_cb, cb_data);
}

int lcc_cdi_control_to_xml(struct lcc_cdi_control* cdi_control, lcc_cdi_xml_data xml_cb, void* cb_data){
    cdi_buffer_loc = 0;

    if(cdi_control == NULL || xml_cb == NULL){
        return LCC_ERRCODE_INVALID_ARG;
    }

    append_string("<?xml version=\"1.0\"?>", xml_cb, cb_data);
    append_string("<cdi xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xsi:noNamespaceSchemaLocation=\"http://openlcb.org/schema/cdi/1/1/cdi.xsd\">", xml_cb, cb_data);

    if(cdi_control->simple_info){
        append_string("<identification>", xml_cb, cb_data);
        append_string("<manufacturer>", xml_cb, cb_data);
        append_string(cdi_control->simple_info->manufacturer_name, xml_cb, cb_data);
        append_string("</manufacturer>", xml_cb, cb_data);
        append_string("<model>", xml_cb, cb_data);
        append_string(cdi_control->simple_info->model_name, xml_cb, cb_data);
        append_string("</model>", xml_cb, cb_data);
        append_string("<hardwareVersion>", xml_cb, cb_data);
        append_string(cdi_control->simple_info->hw_version, xml_cb, cb_data);
        append_string("</hardwareVersion>", xml_cb, cb_data);
        append_string("<softwareVersion>", xml_cb, cb_data);
        append_string(cdi_control->simple_info->sw_version, xml_cb, cb_data);
        append_string("</softwareVersion>", xml_cb, cb_data);
        append_string("</identification>", xml_cb, cb_data);
    }

    struct lcc_cdi_segment* segment = cdi_control->segments;
    while(segment != NULL){
        if(segment == NULL || segment->segment_name == NULL){
            break;
        }

        snprintf(tmp_buffer, sizeof(tmp_buffer), "<segment space='%d'>", segment->segment_space);
        append_string(tmp_buffer, xml_cb, cb_data);
        append_string("<name>", xml_cb, cb_data);
        append_string(segment->segment_name, xml_cb, cb_data);
        append_string("</name>", xml_cb, cb_data);

        // Output all of our groups
        struct lcc_cdi_group* group = segment->groups;
        while(group != NULL){
            output_group(group, xml_cb, cb_data);
            group++;
        }

        // Output all of our inputs
        struct lcc_cdi_input* input = segment->inputs;
        while(input != NULL){
            output_input(input, xml_cb, cb_data);
            input++;
        }

        append_string("</segment>", xml_cb, cb_data);
        segment++;
    }

    append_string("</cdi>", xml_cb, cb_data);

    if(cdi_buffer_loc > 0){
        xml_cb(cdi_buffer, cdi_buffer_loc, cb_data);
    }

    return LCC_OK;
}

int lcc_cdi_control_xml_length(struct lcc_cdi_control* cdi_control){
    if(cdi_control == NULL){
        return LCC_ERRCODE_INVALID_ARG;
    }

    int len = 0;
    int ret = lcc_cdi_control_to_xml(cdi_control, lcc_cdi_xml_data_count, &len);
    if(ret < 0){
        return ret;
    }

    return len;
}
