#ifndef MONICA_H
#define MONICA_H

#define MONICA_MQTT_ADDR        "fd17:cafe:cafe:3::1"
#define MONICA_MQTT_PORT        (1885U)
#define MONICA_MQTT_SIZE        (64U)
#define MONICA_MQTT_STACKSIZE   (3*THREAD_STACKSIZE_DEFAULT)

int sensor_get_temperature(void);
int sensor_get_humidity(void);
size_t node_get_info(char *buf);

typedef struct monica_pub {
    char *topic;
    char *message;
} monica_pub_t;

#endif /* MONICA_H */
