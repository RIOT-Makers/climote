#include "log.h"
#include "thread.h"

#include "msg.h"

#define SENS_MSG_QUEUE_SIZE     (4U)
#define SENS_THREAD_STACKSIZE   (THREAD_STACKSIZE_DEFAULT)

static char sensor_thread_stack[SENS_THREAD_STACKSIZE];
//static msg_t sensor_thread_msg_queue[SENS_MSG_QUEUE_SIZE];

/**
 * @brief udp receiver thread function
 *
 * @param[in] arg   unused
 */
static void *sensor_thread(void *arg)
{
    (void) arg;
    return NULL;
}

/**
 * @brief start sensor thread
 *
 * @return PID of sensor thread
 */
int sensor_init(void)
{
    // start thread
    return thread_create(sensor_thread_stack, sizeof(sensor_thread_stack),
                         THREAD_PRIORITY_MAIN, THREAD_CREATE_STACKTEST,
                         sensor_thread, NULL, "sensor_thread");
}
