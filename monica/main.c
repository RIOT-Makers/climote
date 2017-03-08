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
// own
#include "monica.h"

#ifndef BUTTON_MODE
#define BUTTON_MODE     (GPIO_IN_PU)
#endif

#ifndef BUTTON_GPIO
#define BUTTON_GPIO     (BTN0_PIN)
#endif

#define COMM_PAN        (0x17) // lowpan ID
#define COMM_CHAN       (17U)  // channel

int sensor_pid = -1;
int coap_pid = -1;
int mqtt_pid = -1;

//extern int coap_cmd(int argc, char **argv);
extern int sensor_init(void);
extern int coap_init(void);
extern int mqtt_init(void);

static int cmd_btn(int argc, char **argv);

// array with available shell commands
static const shell_command_t shell_commands[] = {
    { "btn", "soft trigger button", cmd_btn },
    { NULL, NULL, NULL }
};

static void button_cb(void *arg)
{
    LOG_INFO("button_cb: external interrupt (%i)\n", (int)arg);
    msg_t m;
    char mmsg[MONICA_MQTT_SIZE];
    sprintf(mmsg, "This is RIOT on board %s", RIOT_BOARD);
    monica_pub_t mpt = { .topic = "monica/info", .message = mmsg };
    m.content.ptr = &mpt;
    msg_send(&m, mqtt_pid);
#ifdef BOARD_PBA_D_01_KW2X
#endif /* BOARD_PBA_D_01_KW2X */
}

static int button_init(void)
{
    LOG_DEBUG("button_init: enter\n");
    int pi = 314;
    if (gpio_init_int(BUTTON_GPIO, GPIO_IN, GPIO_FALLING, button_cb, (void *)pi) < 0) {
        LOG_ERROR("button_init: failed!\n");
        return 1;
    }
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
    button_cb(NULL);
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
    // start mqtt thread
    LOG_INFO(".. init mqtt\n");
    if((mqtt_pid = mqtt_init()) < 0) {
        return 1;
    }
    // start mqtt thread
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
