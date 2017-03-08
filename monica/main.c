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

int sensor_pid = -1;
int coap_pid = -1;
int mqtt_pid = -1;

extern int coap_init(void);
extern int mqtt_init(void);
extern int sensor_init(void);

static int cmd_btn(int argc, char **argv);

// array with available shell commands
static const shell_command_t shell_commands[] = {
    { "btn", "soft trigger button", cmd_btn },
    { NULL, NULL, NULL }
};

static void button_cb(void *arg)
{
    LOG_INFO("button_cb: external interrupt (%i)\n", *(int *)arg);
    if (mqtt_pid > 0) {
        msg_t mi, mt, mh;
        char mmsg[MONICA_MQTT_SIZE];
        /* publish riot info */
        sprintf(mmsg, "board %s", RIOT_BOARD);
        monica_pub_t mpt_info = { .topic = "monica/info", .message = mmsg };
        mi.content.ptr = &mpt_info;
        msg_send(&mi, mqtt_pid);
        /* publish temperature */
        memset(mmsg, 0, MONICA_MQTT_SIZE);
        sprintf(mmsg, "temperature: %d", sensor_get_temperature());
        monica_pub_t mpt_temp = { .topic = "monica/climate", .message = mmsg };
        mt.content.ptr = &mpt_temp;
        msg_send(&mt, mqtt_pid);
        /* publish humidity */
        memset(mmsg, 0, MONICA_MQTT_SIZE);
        sprintf(mmsg, "humidity: %d", sensor_get_humidity());
        monica_pub_t mpt_hum = { .topic = "monica/climate", .message = mmsg };
        mh.content.ptr = &mpt_hum;
        msg_send(&mh, mqtt_pid);
    }
    else {
        // start mqtt thread
        LOG_INFO(".. init mqtt\n");
        if((mqtt_pid = mqtt_init()) < 0) {
            return 1;
        }
    }
}

static int button_init(void)
{
    LOG_DEBUG("button_init: enter\n");
#ifndef BOARD_NATIVE
    int pi = 314;
    if (gpio_init_int(BUTTON_GPIO, GPIO_IN, GPIO_FALLING, button_cb, (void *)pi) < 0) {
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
    int test = 24911;
    button_cb(&test);
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
    if((coap_pid = coap_init()) < 0) {
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
