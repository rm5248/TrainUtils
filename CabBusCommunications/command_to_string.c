#include "cab_commands.h"

const char* cabbus_command_to_string( int command ){
	switch( command ){
	case CAB_CMD_NONE:
		return "CabCommandNone";
	case CAB_CMD_SEL_LOCO:
		return "CabCommandSelectLoco";
	case CAB_CMD_SWITCH:
		return "CabCommandSwitch";
	case CAB_CMD_MACRO:
		return "CabCommandMacro";
	case CAB_CMD_RESPONSE:
		return "CabCommandResponse";
	case CAB_CMD_SPEED:
		return "CabCommandSpeed";
	case CAB_CMD_DIRECTION:
		return "CabCommandDirection";
	case CAB_CMD_ESTOP:
		return "CabCommandESTOP";
    case CAB_CMD_FUNCTION:
        return "CabCommandFunction";
    case CAB_CMD_UNSELECT_LOCO:
        return "CabCommandUnselectLoco";
	default:
		return "UNKNOWN";
	}
}
