#ifndef MONICA_H
#define MONICA_H

#define MONICA_MQTT_ADDR        "fd17:cafe:cafe:3::1"
#define MONICA_MQTT_PORT        (1885)
#define MONICA_MQTT_SIZE        (64U)
#define MONICA_MQTT_STACKSIZE   (THREAD_STACKSIZE_DEFAULT)

typedef struct monica_pub {
    char *topic;
    char *message;
} monica_pub_t;

#endif /* MONICA_H */
