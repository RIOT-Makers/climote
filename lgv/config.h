#ifndef CONFIG_H
#define CONFIG_H

#include "xtimer.h"

//#define CONFIG_PROXY_ADDR          "fd16:abcd:ef21:3::1"
#define CONFIG_PROXY_ADDR           "fe80::1ac0:ffee:c0ff:ee21"
#define CONFIG_PROXY_PORT           "5683"
#define CONFIG_PATH_TEMPERATURE     "/Observations"
//#define CONFIG_PATH_HUMITIDY       "/Datastreams(3)/Observations"
#define CONFIG_LOOP_WAIT            (10 * US_PER_SEC)
#define CONFIG_STRBUF_LEN           (32U)

int sensor_get_temperature(void);
int sensor_get_humidity(void);
size_t node_get_info(char *buf);

#endif /* CONFIG_H */
