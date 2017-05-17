/**
 * @ingroup     climote
 * @{
 *
 * @file
 * @brief       Implements sensor control
 *
 * @author      smlng <s@mlng.net>
 *
 * @}
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "periph_conf.h"
#include "log.h"
#include "mutex.h"
#include "thread.h"
#include "xtimer.h"

#ifdef MODULE_HDC1000
#include "hdc1000.h"
#include "hdc1000_params.h"
static hdc1000_t dev_hdc1000;
#endif

#ifdef MODULE_TMP006
#include "tmp006.h"
static tmp006_t dev_tmp006;
#endif

#if !defined(MODULE_HDC1000) || !defined(MODULE_TMP006)
#include "random.h"
#endif

#define SENSOR_TIMEOUT_MS       (5 * US_PER_SEC)
#define SENSOR_NUM_SAMPLES      (10U)
#define SENSOR_THREAD_STACKSIZE (3 * THREAD_STACKSIZE_DEFAULT)

static int16_t samples_humidity[SENSOR_NUM_SAMPLES];
static int16_t samples_temperature[SENSOR_NUM_SAMPLES];
static mutex_t mutex;

static char sensor_thread_stack[SENSOR_THREAD_STACKSIZE];

/**
 * @brief get avg temperature over N samples in Celcius (C) with factor 100
 *
 * @return temperature
 */
int sensor_get_temperature(void)
{
    int32_t sum = 0;
    mutex_lock(&mutex);
    for (unsigned i = 0; i < SENSOR_NUM_SAMPLES; i++) {
        sum += samples_temperature[i];
    }
    mutex_unlock(&mutex);
    return (int)(sum/SENSOR_NUM_SAMPLES);
}

/**
 * @brief get avg humitity over N sampels in percent (%) with factor 100
 *
 * @return humidity
 */
int sensor_get_humidity(void)
{
    int32_t sum = 0;
    mutex_lock(&mutex);
    for (unsigned i = 0; i < SENSOR_NUM_SAMPLES; i++) {
        sum += samples_humidity[i];
    }
    mutex_unlock(&mutex);
    return (int)(sum/SENSOR_NUM_SAMPLES);
}

/**
 * @brief Measures the temperature and humitity with a HDC1000.
 *
 * @param[out] temp the measured temperature in degree celsius * 100
 * @param[out] hum the measured humitity in % * 100
 */
static int16_t _get_humidity(void) {
    int16_t h;
#ifdef MODULE_HDC1000
    int16_t t;
    LOG_DEBUG("[SENSOR] _hdc1000_measure\n");
    hdc1000_read(&dev_hdc1000, &t, &h);
#else
    h = (int16_t) random_uint32_range(0, 10000);
#endif /* MODULE_HDC1000 */
    return h;
}

/**
 * @brief Measures the temperature with a TMP006.
 *
 * @param[out] temp the measured temperature in degree celsius * 100
 */
static int16_t _get_temperature(void)
{
    LOG_DEBUG("[SENSOR] _get_temperature\n");
    int16_t t;
#ifdef MODULE_TMP006
    uint8_t drdy;
    int16_t raw_temp, raw_volt;
    float tamb, tobj;
    /* read sensor, quit on error */
    if (tmp006_read(&dev_tmp006, &raw_volt, &raw_temp, &drdy)) {
        LOG_ERROR("[SENSOR] tmp006_read failed\n");
        return 0;
    }
    tmp006_convert(raw_volt, raw_temp,  &tamb, &tobj);
    t = (int16_t)(tobj*100);
#else
    t = (int16_t) random_uint32_range(0, 5000);
#endif /* MODULE_TMP006 */
    return t;
}

/**
 * @brief Intialise all sensores.
 *
 * @return 0 on success, anything else on error
 */
static int _init(void) {
    LOG_DEBUG("[SENSOR] _init\n");
#ifdef MODULE_HDC1000
    /* initialise humidity sensor hdc1000 */
    if ((hdc1000_init(&dev_hdc1000, &hdc1000_params[0]) != 0)) {
        LOG_ERROR("[SENSOR] HDC1000 init");
        return 1;
    }
#endif /* MODULE_HDC1000 */
#ifdef MODULE_TMP006
    /* init temperature sensor tmp006 */
    if ((tmp006_init(&dev_tmp006, TMP006_I2C, TMP006_ADDR, TMP006_CONFIG_CR_DEF) != 0)) {
        LOG_ERROR("[SENSOR] TMP006 init");
        return 1;
    }
    if (tmp006_set_active(&dev_tmp006)) {
        LOG_ERROR("[SENSOR] TMP006 activate.");
        return 1;
    }
    if (tmp006_test(&dev_tmp006)) {
        LOG_ERROR("[SENSOR] TMP006 test.");
        return 1;
    }
#endif /* MODULE_TMP006 */
    mutex_lock(&mutex);
    for (unsigned i = 0; i < SENSOR_NUM_SAMPLES; i++) {
        samples_humidity[i]    = _get_humidity();
        samples_temperature[i] = _get_temperature();
    }
    mutex_unlock(&mutex);
    return 0;
}

/**
 * @brief udp receiver thread function
 *
 * @param[in] arg   unused
 */
static void *sensor_thread(void *arg)
{
    (void) arg;
    int count = 0;
    xtimer_usleep(SENSOR_TIMEOUT_MS);
    while(1) {
        /* get latest sensor data */
        mutex_lock(&mutex);
        samples_temperature[count] = _get_temperature();
        samples_humidity[count] = _get_humidity();
        mutex_unlock(&mutex);
        /* next round */
        count = (count+1)%SENSOR_NUM_SAMPLES;
        if (count == 0) {
            LOG_INFO("[SENSOR] raw data T: %d, H: %d\n",
                     sensor_get_temperature(), sensor_get_humidity());
        }
        xtimer_usleep(SENSOR_TIMEOUT_MS);
    }
    return NULL;
}

/**
 * @brief start udp receiver thread
 *
 * @return PID of sensor control thread
 */
int sensor_init(void)
{
    /* init sensors */
    if (_init() != 0) {
        return -1;
    }
    /* start sensor thread for periodic measurements */
    return thread_create(sensor_thread_stack, sizeof(sensor_thread_stack),
                        THREAD_PRIORITY_MAIN-1, THREAD_CREATE_STACKTEST,
                        sensor_thread, NULL, "sensor_thread");
}
