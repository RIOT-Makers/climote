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
#include "xtimer.h"
// own
#include "config.h"

#define COMM_PAN        (0x2121) // lowpan ID
#define COMM_CHAN       (15U)  // channel

static int sensor_pid = -1;

extern int coap_init(void);
extern int sensor_init(void);
extern void post_sensordata(char *data, char *path);

size_t node_get_info(char *buf)
{
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
                break;
            }
        }
    }
    return len;
}

static int comm_init(void)
{
    kernel_pid_t ifs[GNRC_NETIF_NUMOF];
    uint16_t pan = COMM_PAN;
    uint16_t chan = COMM_CHAN;
    /* get the PID of the first radio */
    if (gnrc_netif_get(ifs) <= 0) {
        LOG_ERROR("!! comm_init failed, not radio found !!\n");
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
    puts(" LGV RIOT Demo - showing CoAP and MQTT ");
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
    // start shell
    LOG_INFO(".. init shell\n");
    LED0_OFF;
#if (defined(LED1_ON) && defined(LED2_ON))
    LED1_OFF;
    LED2_OFF;
#endif
    LOG_INFO("\n");
    while(1) {
        char strbuf[8];
        int t = sensor_get_temperature();
        int h = sensor_get_humidity();
        printf("T: %d, H: %d\n", t,h);
        sprintf(strbuf, "{\"result\":%d}", t);
        puts("> post temperature");
        post_sensordata(strbuf, LGV_PATH_TEMPERATURE);
        sprintf(strbuf, "{\"result\":%d}", h);
        puts("> post humidity");
        post_sensordata(strbuf, LGV_PATH_HUMITIDY);
        xtimer_usleep(LGV_LOOP_WAIT);
    }
    // should be never reached
    return 0;
}
