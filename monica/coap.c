#include "log.h"
#include "msg.h"
#include "thread.h"
//#include "net/gcoap.h"
// own
#include "monica.h"

#define COAP_THREAD_STACKSIZE   (THREAD_STACKSIZE_DEFAULT)

static char coap_thread_stack[COAP_THREAD_STACKSIZE];

/**
 * @brief udp receiver thread function
 *
 * @param[in] arg   unused
 */
static void *coap_thread(void *arg)
{
    (void) arg;
    return NULL;
}

/**
 * @brief start CoAP thread
 *
 * @return PID of CoAP thread
 */
int coap_init(void)
{
    // start thread
    return thread_create(coap_thread_stack, sizeof(coap_thread_stack),
                         THREAD_PRIORITY_MAIN-2, THREAD_CREATE_STACKTEST,
                         coap_thread, NULL, "coap_thread");
}
