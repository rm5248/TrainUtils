/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lcc.h"
#include "lcc-memory.h"
#include "lcc-datagram.h"
#include "lcc-cdi-control.h"
#include "lcc-common.h"

// Include our internal header for the tests
#define LIBLCC_BUILD
#include "lcc-common-internal.h"

#include "test-common.h"

struct test_file_info{
    FILE* file;
    char file_name[64];
};

static void xml_data_cb(const char* str, int len, void* cb_data){
    struct test_file_info* inf = cb_data;
    fwrite(str, 1, len, inf->file);
}

static struct test_file_info create_tmp_file_and_write(const char* xml){
    struct test_file_info ret = {0};
    char template[] = "/tmp/trainutils-XXXXXX";
    int fd = mkstemp(template);
    if(fd < 0){
        fprintf(stderr, "unable to open tmp file");
        return ret;
    }
    FILE* tmp_file = fdopen(fd, "w");
    ret.file = tmp_file;
    strcpy(ret.file_name, template);

    if(xml != NULL){
        fwrite(xml, 1, strlen(xml), tmp_file);
        fflush(tmp_file);
    }

    return ret;
}

static void delete_file(struct test_file_info inf){
    fclose(inf.file);
    unlink(inf.file_name);
}

static int control1(void){
    const char* xml = "<cdi xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='http://openlcb.org/schema/cdi/1/1/cdi.xsd'> \
    <identification> \
    <manufacturer>Some NMRA Manufacturer</manufacturer> \
    <model>An Excellent Model</model> \
    <hardwareVersion>1.0</hardwareVersion> \
    <softwareVersion>2.22</softwareVersion> \
    </identification> \
    </cdi>";

    struct lcc_cdi_control cdi_control = {0};
    struct lcc_simple_node_info node_info = {0};
    cdi_control.simple_info = &node_info;
    strcpy(node_info.manufacturer_name, "Some NMRA Manufacturer");
    strcpy(node_info.model_name, "An Excellent Model");
    strcpy(node_info.hw_version, "1.0");
    strcpy(node_info.sw_version, "2.22");

    struct test_file_info expected = create_tmp_file_and_write(xml);
    struct test_file_info actual = create_tmp_file_and_write(NULL);

    lcc_cdi_control_to_xml(&cdi_control, xml_data_cb, &actual);
    fflush(actual.file);

    char to_exec[512];
    snprintf(to_exec, sizeof(to_exec), "xml-logical-difftool/logic-xml-diff.sh %s %s", expected.file_name, actual.file_name);
    int val = system(to_exec);

    delete_file(expected);
    delete_file(actual);

    return WEXITSTATUS(val);
}

static int control2(void){
    const char* xml = "<?xml version='1.0'?> \
            <cdi xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='http://openlcb.org/schema/cdi/1/1/cdi.xsd'> \
            <identification> \
            <manufacturer>An NMRA MFG</manufacturer> \
            <model>X-GATE</model> \
            <hardwareVersion>1</hardwareVersion> \
            <softwareVersion>abc</softwareVersion> \
            </identification> \
            <segment space='253'> \
            <name>Occupation Events</name> \
            <group> \
            <name>Occupation events(left-to-right)</name> \
            <eventid> \
            <name>Occupation pre-island LTR event</name> \
            <description>When a train enters the block before the island(left-to-right)</description> \
            </eventid> \
            <eventid> \
            <name>Occupation island LTR event</name> \
            <description>When a train enters the island(left-to-right)</description> \
            </eventid> \
            <eventid> \
            <name>Occupation post-island LTR event</name> \
            <description>When a train enters the leaves(left-to-right)</description> \
            </eventid> \
            <eventid> \
            <name>Occupation Unoccupied LTR event</name> \
            <description>When track is now unoccupied, this event is sent(left-to-right)</description> \
            </eventid> \
            </group> \
            <group> \
            <name>Occupation events(right-to-left)</name> \
            <eventid> \
            <name>Occupation pre-island RTL event</name> \
            <description>When a train enters the block before the island(right-to-left)</description> \
            </eventid> \
            <eventid> \
            <name>Occupation island RTL event</name> \
            <description>When a train enters the island(right-to-left)</description> \
            </eventid> \
            <eventid> \
            <name>Occupation post-island RTL event</name> \
            <description>When a train enters the leaves(right-to-left)</description> \
            </eventid> \
            <eventid> \
            <name>Occupation Unoccupied RTL event</name> \
            <description>When track is now unoccupied, this event is sent(right-to-left)</description> \
            </eventid> \
            </group> \
            </segment> \
            <segment space='251'> \
            <name>Node ID</name> \
            <group> \
            <name>Your name and description for this node</name> \
            <string size='63'> \
            <name>Node Name</name> \
            </string> \
            <string size='64' offset='1'> \
            <name>Node Description</name> \
            </string> \
            </group> \
            </segment> \
            </cdi>";

    struct lcc_cdi_control cdi_control = {0};
    struct lcc_cdi_input segment_253_ltr_inputs[] = {
        {"Occupation pre-island LTR event", "When a train enters the block before the island(left-to-right)",
            NULL, 0, 0, LCC_CDI_INPUT_TYPE_EVENTID, 0, 0},
        {"Occupation island LTR event", "When a train enters the island(left-to-right)",
            NULL, 0, 0, LCC_CDI_INPUT_TYPE_EVENTID, 0, 0},
        {"Occupation post-island LTR event", "When a train enters the leaves(left-to-right)",
            NULL, 0, 0, LCC_CDI_INPUT_TYPE_EVENTID, 0, 0},
        {"Occupation Unoccupied LTR event", "When track is now unoccupied, this event is sent(left-to-right)",
            NULL, 0, 0, LCC_CDI_INPUT_TYPE_EVENTID, 0, 0},
        NULL
    };
    struct lcc_cdi_input segment_253_rtl_inputs[] = {
        {"Occupation pre-island RTL event", "When a train enters the block before the island(right-to-left)",
            NULL, 0, 0, LCC_CDI_INPUT_TYPE_EVENTID, 0, 0},
        {"Occupation island RTL event", "When a train enters the island(right-to-left)",
            NULL, 0, 0, LCC_CDI_INPUT_TYPE_EVENTID, 0, 0},
        {"Occupation post-island RTL event", "When a train enters the leaves(right-to-left)",
            NULL, 0, 0, LCC_CDI_INPUT_TYPE_EVENTID, 0, 0},
        {"Occupation Unoccupied RTL event", "When track is now unoccupied, this event is sent(right-to-left)",
            NULL, 0, 0, LCC_CDI_INPUT_TYPE_EVENTID, 0, 0},
        NULL
    };
    struct lcc_cdi_group segment_253_groups[] = {
        {"Occupation events(left-to-right)", NULL, 0, 0, segment_253_ltr_inputs},
        {"Occupation events(right-to-left)", NULL, 0, 0, segment_253_rtl_inputs},
        NULL
    };
    struct lcc_cdi_segment segment_253 = {
        .segment_space = 253,
        .segment_name = "Occupation Events",
        .groups = segment_253_groups,
        .inputs = NULL
    };
    struct lcc_cdi_input segment_251_inputs[] = {
        {"Node Name", NULL, NULL, 0, 0, LCC_CDI_INPUT_TYPE_STRING, 63, 0},
        {"Node Description", NULL, NULL, 0, 0, LCC_CDI_INPUT_TYPE_STRING, 64, 1},
        NULL
    };
    struct lcc_cdi_group segment_251_groups[] = {
        {"Your name and description for this node", NULL, 0, 0, segment_251_inputs},
        NULL
    };
    struct lcc_cdi_segment segment_251 = {
        .segment_space = 251,
        .segment_name = "Node ID",
        .groups = segment_251_groups,
        .inputs = NULL
    };
    struct lcc_cdi_segment segments[] = {
        segment_253,
        segment_251,
        NULL
    };
    struct lcc_simple_node_info node_info = {0};
    cdi_control.simple_info = &node_info;
    strcpy(node_info.manufacturer_name, "An NMRA MFG");
    strcpy(node_info.model_name, "X-GATE");
    strcpy(node_info.hw_version, "1");
    strcpy(node_info.sw_version, "abc");
    cdi_control.segments = segments;

    struct test_file_info expected = create_tmp_file_and_write(xml);
    struct test_file_info actual = create_tmp_file_and_write(NULL);

    lcc_cdi_control_to_xml(&cdi_control, xml_data_cb, &actual);
    fflush(actual.file);

    char to_exec[512];
    snprintf(to_exec, sizeof(to_exec), "xml-logical-difftool/logic-xml-diff.sh %s %s", expected.file_name, actual.file_name);
    int val = system(to_exec);

    delete_file(expected);
    delete_file(actual);

    return WEXITSTATUS(val);
}

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "control1") == 0){
        return control1();
    }else if(strcmp(argv[1], "control2") == 0){
        return control2();
    }

    return 1;
}
