/**
 * @ingroup     climote
 * @{
 *
 * @file
 * @brief       Main control loop
 *
 * @author      smlng <s@mlng.net>
 *
 * @}
 */

// standard
 #include <inttypes.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
// riot
#include "board.h"
#include "log.h"
#include "msg.h"
#include "net/af.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc/netif.h"
#include "periph/gpio.h"
#include "shell.h"
#include "xtimer.h"
// own
#include "monica.h"

#ifndef BUTTON_MODE
#define BUTTON_MODE     (GPIO_IN_PU)
#endif

#if !defined(BUTTON_GPIO) && !defined(BOARD_NATIVE)
#define BUTTON_GPIO     (BTN0_PIN)
#endif

#define COMM_PAN        (0x17) // lowpan ID
#define COMM_CHAN       (17U)  // channel

static int sensor_pid = -1;
static int mqtt_pid = -1;
static int btn_pid = -1;

extern int coap_init(void);
extern int mqtt_init(void);
extern int sensor_init(void);

static int cmd_btn(int argc, char **argv);
static char btn_thread_stack[MONICA_MQTT_STACKSIZE];
msg_t btn_msg;

// array with available shell commands
static const shell_command_t shell_commands[] = {
    { "btn", "soft trigger button", cmd_btn },
    { NULL, NULL, NULL }
};

size_t node_get_info(char *buf)
{
    //size_t len = sprintf(buf, "{'board': '%s'", RIOT_BOARD);
    size_t len = 0;
    kernel_pid_t ifs[GNRC_NETIF_NUMOF];
    size_t numof = gnrc_netif_get(ifs);
    if (numof > 0) {
        gnrc_ipv6_netif_t *entry = gnrc_ipv6_netif_get(ifs[0]);
        char ipv6_addr_str[IPV6_ADDR_MAX_STR_LEN];
        for (int i = 0; i < GNRC_IPV6_NETIF_ADDR_NUMOF; i++) {
            if (ipv6_addr_is_global(&entry->addrs[i].addr) &&
                    !ipv6_addr_is_multicast(&entry->addrs[i].addr) &&
                    !(entry->addrs[i].flags & GNRC_IPV6_NETIF_ADDR_FLAGS_NON_UNICAST)) {
                ipv6_addr_to_str(ipv6_addr_str, &entry->addrs[i].addr, IPV6_ADDR_MAX_STR_LEN);
                //len += sprintf(buf+len, ", 'addr': '%s'", ipv6_addr_str);
                len = sprintf(buf, "{'addr': '%s'}", ipv6_addr_str);
                return len;
            }
        }
    }
    len += sprintf(buf+len, "}");
    return len;
}

/**
 * @brief udp receiver thread function
 *
 * @param[in] arg   unused
 */
static void *btn_thread(void *arg)
{
    (void) arg;
    static msg_t msgq[4];
    msg_init_queue(msgq, 4);

    while(1) {
        msg_t m;
        msg_receive(&m);
        if (mqtt_pid > 0) {
            msg_t req, resp;
            /* publish riot info */
            char buf[MONICA_MQTT_SIZE];
            memset(buf, 0, MONICA_MQTT_SIZE);
            size_t len = node_get_info(buf);
            (void) len;
            monica_pub_t mpt_info = { .topic = "monica/info", .message = buf };
            req.content.ptr = &mpt_info;
            msg_send_receive(&req, &resp, mqtt_pid);
            /* publish climate data */
            memset(buf, 0, MONICA_MQTT_SIZE);
            sprintf(buf, "{'temperature': %d, 'humidity': %d}", sensor_get_temperature(), sensor_get_humidity());
            monica_pub_t mpt_climate = { .topic = "monica/climate", .message = buf };
            req.content.ptr = &mpt_climate;
            msg_send_receive(&req, &resp, mqtt_pid);
        }
        else {
            // start mqtt thread
            LOG_INFO(".. init mqtt.\n");
            if((mqtt_pid = mqtt_init()) < 0) {
                LOG_ERROR("!! init mqtt failed !!\n");
            }
        }
    }
    return NULL;
}
static void button_cb(void *arg)
{
    (void) arg;
    LOG_INFO("button_cb: external interrupt.\n");
    msg_t *msg = (msg_t*)arg;
    msg_send_int(msg, btn_pid);
}

static int button_init(void)
{
    LOG_DEBUG("button_init: enter\n");
    btn_pid = thread_create(btn_thread_stack, sizeof(btn_thread_stack),
                            THREAD_PRIORITY_MAIN, THREAD_CREATE_STACKTEST,
                            btn_thread, NULL, "btn_thread");
#ifndef BOARD_NATIVE
    if (gpio_init_int(BUTTON_GPIO, BUTTON_MODE, GPIO_FALLING, button_cb, (void *)&btn_msg) < 0) {
        LOG_ERROR("button_init: failed!\n");
        return 1;
    }
#endif /* BOARD_NATIVE */
    return 0;
}

static int comm_init(void)
{
    LOG_DEBUG("comm_init: enter\n");
    kernel_pid_t ifs[GNRC_NETIF_NUMOF];
    uint16_t pan = COMM_PAN;
    uint16_t chan = COMM_CHAN;
    /* get the PID of the first radio */
    if (gnrc_netif_get(ifs) <= 0) {
        LOG_ERROR("comm_init: not radio found!\n");
        return 1;
    }
    /* initialize the radio */
    gnrc_netapi_set(ifs[0], NETOPT_NID, 0, &pan, 2);
    gnrc_netapi_set(ifs[0], NETOPT_CHANNEL, 0, &chan, 2);
    return 0;
}

int cmd_btn(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    msg_send(&btn_msg, btn_pid);
    return 0;
}
/**
 * @brief the main programm loop
 *
 * @return non zero on error
 */
int main(void)
{
    // some initial infos
    puts(" MONICA RIOT Demo - showing CoAP and MQTT ");
    puts("==========================================\n");
    // init 6lowpan interface
    LED0_ON;
    LOG_INFO(".. init network\n");
    if (comm_init() != 0) {
        return 1;
    }
    // start sensor thread
    LOG_INFO(".. init sensors\n");
    if ((sensor_pid = sensor_init()) < 0) {
        return 1;
    }
    // start coap thread
    LOG_INFO(".. init coap\n");
    if(coap_init() < 0) {
        return 1;
    }
    // init button
    LOG_INFO(".. init button\n");
    if(button_init() != 0) {
        return 1;
    }
    // start shell
    LOG_INFO(".. init shell\n");
    LED0_OFF;
#if (defined(LED1_ON) && defined(LED2_ON))
    LED1_OFF;
    LED2_OFF;
#endif
    LOG_INFO("\n");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    // should be never reached
    return 0;
}
