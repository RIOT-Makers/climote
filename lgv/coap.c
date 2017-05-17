#include "log.h"
#include "msg.h"
#include "thread.h"
#include "od.h"
#include "net/gcoap.h"
// own
#include "config.h"

static ssize_t _info_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len);
static ssize_t _climate_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len);
/*
enum {
    COAP_METHOD_NONE,
    COAP_METHOD_GET,
    COAP_METHOD_POST,
    COAP_METHOD_PUT,
    COAP_METHOD_DELETE
}
*/

/* Counts requests sent by CLI. */
static uint16_t req_count = 0;

/* CoAP resources */
static const coap_resource_t _resources[] = {
    { "/lgv/climate", COAP_GET, _climate_handler },
    { "/lgv/info", COAP_GET, _info_handler },
};

static gcoap_listener_t _listener = {
    (coap_resource_t *)&_resources[0],
    sizeof(_resources) / sizeof(_resources[0]),
    NULL
};

/*
 * Response callback.
 */
static void _resp_handler(unsigned req_state, coap_pkt_t* pdu)
{
    if (req_state == GCOAP_MEMO_TIMEOUT) {
        printf("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu));
        return;
    }
    else if (req_state == GCOAP_MEMO_ERR) {
        printf("gcoap: error in response\n");
        return;
    }

    char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS)
                            ? "Success" : "Error";
    printf("gcoap: response %s, code %1u.%02u", class_str,
                                                coap_get_code_class(pdu),
                                                coap_get_code_detail(pdu));
    if (pdu->payload_len) {
        if (pdu->content_type == COAP_FORMAT_TEXT
                || pdu->content_type == COAP_FORMAT_LINK
                || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE
                || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE) {
            /* Expecting diagnostic payload in failure cases */
            printf(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
                                                          (char *)pdu->payload);
        }
        else {
            printf(", %u bytes\n", pdu->payload_len);
            od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
        }
    }
    else {
        printf(", empty payload\n");
    }
}

static size_t _send(uint8_t *buf, size_t len, char *addr_str, char *port_str)
{
    ipv6_addr_t addr;
    size_t bytes_sent;
    sock_udp_ep_t remote;

    remote.family = AF_INET6;
    remote.netif  = SOCK_ADDR_ANY_NETIF;

    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("gcoap_cli: unable to parse destination address");
        return 0;
    }
    memcpy(&remote.addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

    /* parse port */
    remote.port = (uint16_t)atoi(port_str);
    if (remote.port == 0) {
        puts("gcoap_cli: unable to parse destination port");
        return 0;
    }

    bytes_sent = gcoap_req_send2(buf, len, &remote, _resp_handler);
    if (bytes_sent > 0) {
        req_count++;
    }
    return bytes_sent;
}

/*
 * Server callback for /cli/stats. Returns the count of packets sent by the
 * CLI.
 */
static ssize_t _info_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len)
{
    LOG_DEBUG("[CoAP] info_handler\n");
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);

    size_t payload_len = node_get_info((char *)pdu->payload);

    return gcoap_finish(pdu, payload_len, COAP_FORMAT_JSON);
}

/*
 * Server callback for /cli/stats. Returns the count of packets sent by the
 * CLI.
 */
static ssize_t _climate_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len)
{
    LOG_DEBUG("[CoAP] climate_handler\n");
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);

    size_t payload_len = sprintf((char *)pdu->payload, "{'temperature': %d, 'humidity': %d}", sensor_get_temperature(), sensor_get_humidity());

    return gcoap_finish(pdu, payload_len, COAP_FORMAT_JSON);
}

void post_sensordata(char *data, char *path)
{
    uint8_t buf[GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;

    gcoap_req_init(&pdu, &buf[0], GCOAP_PDU_BUF_SIZE, COAP_METHOD_POST, path);
    memcpy(pdu.payload, data, strlen(data));
    len = gcoap_finish(&pdu, strlen(data), COAP_FORMAT_JSON);
    if (!_send(&buf[0], len, LGV_PROXY_ADDR, LGV_PROXY_PORT)) {
        puts("gcoap_cli: msg send failed");
    }
}

/**
 * @brief start CoAP thread
 *
 * @return PID of CoAP thread
 */
int coap_init(void)
{
    gcoap_register_listener(&_listener);
    return 0;
}
