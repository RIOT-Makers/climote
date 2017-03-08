#include "log.h"
#include "thread.h"
#include "msg.h"
#include "net/emcute.h"
// own
#include "monica.h"

#define MONICA_MQTT_PORT        (1885)
#define MONICA_MQTT_ADDR        "fd17:cafe:cafe:3::1"
#define MQTT_MSG_QUEUE_SIZE     (4U)
#define MQTT_THREAD_STACKSIZE   (THREAD_STACKSIZE_DEFAULT)

static char mqtt_thread_stack[MQTT_THREAD_STACKSIZE];
//static msg_t mqtt_thread_msg_queue[MQTT_MSG_QUEUE_SIZE];

static int con(void)
{
    sock_udp_ep_t gw = { .family = AF_INET6, .port = MONICA_MQTT_PORT };
    char  *will_top = NULL;
    char  *will_msg = NULL;
    size_t will_len = 0;
    /* parse broker address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, MONICA_MQTT_ADDR) == NULL) {
        LOG_ERROR("[MQTT] failed to parse broker address!\n");
        return 1;
    }
    /* connect to broker */
    if (emcute_con(&gw, true, will_top, will_msg, will_len, 0) != EMCUTE_OK) {
        LOG_ERROR("[MQTT] failed to connect with broker!\n");
        return 1;
    }
    return 0;
}

/**
 * @brief udp receiver thread function
 *
 * @param[in] arg   unused
 */
static void *mqtt_thread(void *arg)
{
    (void) arg;
    if (con() != 0) {
        return NULL;
    }
    while(1) {
        msg_t msg;
        msg_receive(&msg);
    }
    return NULL;
}

/**
 * @brief start MQTT thread
 *
 * @return PID of MQTT thread
 */
int mqtt_init(void)
{
    // start thread
    return thread_create(mqtt_thread_stack, sizeof(mqtt_thread_stack),
                         THREAD_PRIORITY_MAIN, THREAD_CREATE_STACKTEST,
                         mqtt_thread, NULL, "mqtt_thread");
}
