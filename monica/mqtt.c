#include "log.h"
#include "msg.h"
#include "net/emcute.h"
#include "net/ipv6/addr.h"
#include "thread.h"
// own
#include "monica.h"

static char mqtt_thread_stack[MONICA_MQTT_STACKSIZE];

static int _con(void)
{
    LOG_INFO("[MQTT] try connect to broker ... ");
    sock_udp_ep_t gw = { .family = AF_INET6, .port = MONICA_MQTT_PORT };
    char *addr = MONICA_MQTT_ADDR;
    char  *will_top = NULL;
    char  *will_msg = NULL;
    size_t will_len = 0;
    /* parse broker address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, addr) == NULL) {
        LOG_ERROR("failed to parse broker address!\n");
        return 1;
    }
    /* connect to broker */
    if (emcute_con(&gw, true, will_top, will_msg, will_len, 0) != EMCUTE_OK) {
        LOG_ERROR("failed!\n");
        return 1;
    }
    LOG_INFO("success.\n");
    return 0;
}

static int _pub(monica_pub_t *mpt)
{
    emcute_topic_t t;
    unsigned flags = EMCUTE_QOS_0;
    /* get topic id */
    t.name = mpt->topic;
    if (emcute_reg(&t) != EMCUTE_OK) {
        LOG_ERROR("[MQTT] pub: unable to obtain topic ID");
        return 1;
    }
    /* publish data */
    if (emcute_pub(&t, mpt->message, strlen(mpt->message), flags) != EMCUTE_OK) {
        LOG_ERROR("[MQTT] pub: unable to publish data to topic '%s [%i]'\n",
                  t.name, (int)t.id);
        return 1;
    }
    LOG_DEBUG("[MQTT] publish success.\n");
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
    if (_con() != 0) {
        return NULL;
    }
    while(1) {
        msg_t msg;
        msg_receive(&msg);
        _pub(msg.content.ptr);
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
                         THREAD_PRIORITY_MAIN-1, THREAD_CREATE_STACKTEST,
                         mqtt_thread, NULL, "mqtt_thread");
}
