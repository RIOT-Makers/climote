#include "log.h"
#include "msg.h"
#include "thread.h"
#include "net/gcoap.h"
// own
#include "monica.h"

static ssize_t _info_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len);
static ssize_t _climate_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len);

/* CoAP resources */
static const coap_resource_t _resources[] = {
    { "/monica/climate", COAP_GET, _climate_handler },
    { "/monica/info", COAP_GET, _info_handler },
};

static gcoap_listener_t _listener = {
    (coap_resource_t *)&_resources[0],
    sizeof(_resources) / sizeof(_resources[0]),
    NULL
};

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
