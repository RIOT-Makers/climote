#include "log.h"
#include "msg.h"
#include "net/emcute.h"
#include "net/ipv6/addr.h"
#include "thread.h"
// own
#include "monica.h"

#define EMCUTE_PORT         (1883U)
#define EMCUTE_ID           ("eve")
#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 1)

static char stack[THREAD_STACKSIZE_DEFAULT];
static char mqtt_thread_stack[MONICA_MQTT_STACKSIZE];
static int emcute_pid = -1;

static int _con(void)
{
    LOG_DEBUG("[MQTT] try connect to broker ...\n");
    sock_udp_ep_t gw = { .family = AF_INET6, .port = MONICA_MQTT_PORT };
    char *addr = "fd17:cafe:cafe:3::1";
    /* parse broker address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, addr) == NULL) {
        LOG_ERROR("[MQTT] failed to parse broker address!\n");
        return 1;
    }
    /* connect to broker */
    if (emcute_con(&gw, true, NULL, NULL, 0, 0) != EMCUTE_OK) {
        LOG_ERROR("[MQTT] failed to connect to broker!\n");
        return 1;
    }
    LOG_DEBUG(" ... successs.\n");
    return 0;
}

static int _pub(monica_pub_t *mpt)
{
    LOG_DEBUG("[MQTT] pub (%s,%s)\n", mpt->topic, mpt->message);
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

static void *emcute_thread(void *arg)
{
    (void)arg;
    emcute_run(EMCUTE_PORT, EMCUTE_ID);
    return NULL;    /* should never be reached */
}

/**
 * @brief udp receiver thread function
 *
 * @param[in] arg   unused
 */
static void *mqtt_thread(void *arg)
{
    (void) arg;

    while(1) {
        msg_t req, resp;
        msg_receive(&req);
        _pub((monica_pub_t *)req.content.ptr);
        msg_reply(&req,&resp);
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
    /* start the emcute thread */
    if (emcute_pid < 0) {
        emcute_pid = thread_create(stack, sizeof(stack), EMCUTE_PRIO, 0, emcute_thread, NULL, "emcute");
    }
    if (_con() != 0) {
        return -1;
    }
    // start thread
    return thread_create(mqtt_thread_stack, sizeof(mqtt_thread_stack),
                         THREAD_PRIORITY_MAIN-1, THREAD_CREATE_STACKTEST,
                         mqtt_thread, NULL, "mqtt_thread");
}
