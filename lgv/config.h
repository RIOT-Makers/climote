#ifndef LGV_H
#define LGV_H

#include "xtimer.h"

#define LGV_PROXY_ADDR          "fd16:abcd:ef21:3::1"
#define LGV_PROXY_PORT          "5683"
#define LGV_PATH_TEMPERATURE    "/Observations"
//#define LGV_PATH_HUMITIDY       "/Datastreams(3)/Observations"
#define LGV_LOOP_WAIT           (10 * US_PER_SEC)

int sensor_get_temperature(void);
int sensor_get_humidity(void);
size_t node_get_info(char *buf);

#endif /* LGV_H */
