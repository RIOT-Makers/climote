#include "log.h"
#include "thread.h"

#include "msg.h"
#include "net/emcute.h"

#define MQTT_MSG_QUEUE_SIZE     (8U)
#define MQTT_THREAD_STACKSIZE   (2 * THREAD_STACKSIZE_DEFAULT)

static char mqtt_thread_stack[MQTT_THREAD_STACKSIZE];
static msg_t mqtt_thread_msg_queue[MQTT_MSG_QUEUE_SIZE];

/**
 * @brief udp receiver thread function
 *
 * @param[in] arg   unused
 */
static void *mqtt_thread(void *arg)
{
    (void) arg;
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
