#ifndef HRS_SERVICE_H
#define HRS_SERVICE_H

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void hrs_init(void);
int bt_hrs_notify(uint16_t heartrate);

#ifdef __cplusplus
}
#endif

#endif /* HRS_SERVICE_H */
