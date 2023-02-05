#ifndef LOCONET_UTIL_H
#define LOCONET_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

struct loconet_context;

int loconet_global_power_on(struct loconet_context* ctx);

int loconet_global_power_off(struct loconet_context* ctx);

#ifdef	__cplusplus
}
#endif

#endif
