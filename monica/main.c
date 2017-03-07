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

// array with available shell commands
static const shell_command_t shell_commands[] = {
//    { "get", "get sensor", cmd_get },
    { NULL, NULL, NULL }
};

static void button_cb(void *arg)
{
    LOG_INFO("button_cb: external interrupt (%i)\n", (int)arg);
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

/**
 * @brief the main programm loop
 *
 * @return non zero on error
 */
int main(void)
{
    // some initial infos
    puts(" CliMoTe - Climate Monitoring Terminal! ");
    puts("========================================");
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);
    puts("========================================");
    // init 6lowpan interface
    LED0_ON;
    LOG_INFO(".. init network\n");
    if (comm_init() != 0) {
        return 1;
    }
    puts(".");
    // start sensor thread
    LOG_INFO(".. init sensors\n");
    if ((sensor_pid = sensor_init()) < 0) {
        return 1;
    }
    puts(".");
    // start coap thread
    LOG_INFO(".. init coap\n");
    if((coap_pid = coap_init()) < 0) {
        return 1;
    }
    puts(".");
    // start mqtt thread
    LOG_INFO(".. init mqtt\n");
    if((mqtt_pid = mqtt_init()) < 0) {
        return 1;
    }
    puts(".");
    // start shell
    LOG_INFO(".. init shell");
    puts(".");
    LED0_OFF;
#if (defined(LED1_ON) && defined(LED2_ON))
    LED1_OFF;
    LED2_OFF;
#endif
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    // should be never reached
    return 0;
}
