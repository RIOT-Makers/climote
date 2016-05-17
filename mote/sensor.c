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
#include "thread.h"
#include "xtimer.h"

#ifndef BOARD_SAMR21_XPRO
#include "periph/adc.h"
#endif

#ifdef MODULE_HDC1000
#include "hdc1000.h"
static hdc1000_t dev_hdc1000;
#endif

#ifdef MODULE_TMP006
#include "tmp006.h"
static tmp006_t dev_tmp006;
#endif

#include "sensor.h"

#define SENSOR_MSG_QUEUE_SIZE   (8U)
#define SENSOR_TIMEOUT_MS       (5000*1000)
#define SENSOR_NUM_SAMPLES      (6U)
#define SENSOR_THREAD_STACKSIZE (THREAD_STACKSIZE_DEFAULT)

static int samples_airquality[SENSOR_NUM_SAMPLES];
static int samples_humidity[SENSOR_NUM_SAMPLES];
static int samples_temperature[SENSOR_NUM_SAMPLES];

static char sensor_thread_stack[SENSOR_THREAD_STACKSIZE];
static msg_t sensor_thread_msg_queue[SENSOR_MSG_QUEUE_SIZE];

/**
 * @brief get avg temperature over N samples in Celcius (C) with factor 100
 *
 * @return temperature
 */
int sensor_get_temperature(void)
{
    int sum = 0;
    for (int i=0; i < SENSOR_NUM_SAMPLES; i++) {
        sum += samples_temperature[i];
    }
    int avg = (sum/SENSOR_NUM_SAMPLES);
    return avg;
}

/**
 * @brief get avg humitity over N sampels in percent (%) with factor 100
 *
 * @return humidity
 */
int sensor_get_humidity(void)
{
    int sum = 0;
    for (int i=0; i < SENSOR_NUM_SAMPLES; i++) {
        sum += samples_humidity[i];
    }
    int avg = (sum/SENSOR_NUM_SAMPLES);
    return avg;
}

/**
 * @brief get avg air quality over N sampels in percent (%) with factor 100
 *
 * @return air quality
 */
int sensor_get_airquality(void)
{
    int sum = 0;
    for (int i=0; i < SENSOR_NUM_SAMPLES; i++) {
        sum += samples_airquality[i];
    }
    int avg = (sum/SENSOR_NUM_SAMPLES);
    return avg;
}

#ifdef MODULE_HDC1000
/**
 * @brief Measures the temperature and humitity with a HDC1000.
 *
 * @param[out] temp the measured temperature in degree celsius * 100
 * @param[out] hum the measured humitity in % * 100
 */
static void sensor_hdc1000_measure(int *temp, int *hum) {
    uint16_t raw_temp, raw_hum;
    /* init measurment */
    if (hdc1000_startmeasure(&dev_hdc1000)) {
        puts("ERROR: HDC1000 measure");
        return;
    }
    /* wait for the measurment to finish */
    xtimer_usleep(HDC1000_CONVERSION_TIME); //26000us
    hdc1000_read(&dev_hdc1000, &raw_temp, &raw_hum);
    hdc1000_convert(raw_temp, raw_hum,  temp, hum);
}
#endif /* MODULE_HDC1000 */

#ifdef MODULE_TMP006
/**
 * @brief Measures the temperature with a TMP006.
 *
 * @param[out] temp the measured temperature in degree celsius * 100
 */
static void sensor_tmp006_measure(int *temp)
{
    uint8_t drdy;
    int16_t raw_temp, raw_volt;
    float tamb, tobj;

    /* read sensor, quit on error */
    if (tmp006_read(&dev_tmp006, &raw_volt, &raw_temp, &drdy)) {
        puts("ERROR: TMP006 measure");
        return;
    }
    tmp006_convert(raw_volt, raw_temp,  &tamb, &tobj);
    *temp = (int)(tobj*100);
}
#endif /* MODULE_TMP006 */

#ifndef BOARD_SAMR21_XPRO
/**
 * @brief Measure air quality using MQ135 via ADC
 *
 * @return raw airQuality value
 */
static void sensor_mq135_measure(int *airq){
    *airq = adc_sample(ADC_LINE(0), ADC_RES_16BIT);
}
#endif /* BOARD_SAMR21_XPRO */

/**
 * @brief Intialise all sensores.
 *
 * @return 0 on success, anything else on error
 */
static int sensor_init(void) {
#ifndef BOARD_SAMR21_XPRO
    if (ADC_NUMOF < 1) {
        puts("ERROR: no ADC device found");
        return 1;
    }
    /* init ADC devices for air quality sensor MQ135 */
    for (int i = 0; i < ADC_NUMOF; i++) {
        if (adc_init(i) != 0) {
            printf("ERROR: init ADC_%d!\n", i);
            return 1;
        }
    }
#endif /* BOARD_SAMR21_XPRO */
#ifdef MODULE_HDC1000
    assert(SENSOR_TIMEOUT_MS > HDC1000_CONVERSION_TIME);
    /* initialise humidity sensor hdc1000 */
    if (!(hdc1000_init(&dev_hdc1000,
                       HDC1000_I2C, HDC1000_I2C_ADDRESS) == 0)) {
        puts("ERROR: HDC1000 init");
        return 1;
    }
#endif /* MODULE_HDC1000 */
#ifdef MODULE_TMP006
    assert(SENSOR_TIMEOUT_MS > TMP006_CONVERSION_TIME);
    /* init temperature sensor tmp006 */
    if (!(tmp006_init(&dev_tmp006, TMP006_I2C,
                      TMP006_ADDR, TMP006_CONFIG_CR_DEF) == 0)) {
        puts("ERROR: TMP006 init");
        return 1;
    }
    if (tmp006_set_active(&dev_tmp006)) {
        puts("ERROR: TMP006 activate.");
        return 1;
    }
    if (tmp006_test(&dev_tmp006)) {
        puts("ERROR: TMP006 test.");
        return 1;
    }
    puts("SUCCESS: TMP006 init and test!");
    xtimer_usleep(TMP006_CONVERSION_TIME);
#endif /* MODULE_TMP006 */
    int h1 = 0;
    int t1 = 0;
    int t2 = 0;
    int a1 = 0;
#ifndef BOARD_SAMR21_XPRO
    sensor_mq135_measure(&a1);
#else
    (void)a1;
#endif /* BOARD_SAMR21_XPRO */
#ifdef MODULE_HDC1000
    sensor_hdc1000_measure(&t1,&h1);
#else
    (void)t1;
    (void)h1;
#endif /* MODULE_HDC1000 */
#ifdef MODULE_TMP006
    sensor_tmp006_measure(&t2);
#else
    (void)t2;
#endif /* MODULE_TMP006 */
    for (int i=0; i<SENSOR_NUM_SAMPLES; i++) {
        samples_airquality[i] = a1;
        samples_humidity[i] = h1;
        samples_temperature[i] = t2;
    }
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
    msg_init_queue(sensor_thread_msg_queue, SENSOR_MSG_QUEUE_SIZE);
    xtimer_usleep(SENSOR_TIMEOUT_MS);
    while(1) {
#ifdef MODULE_TMP006
        sensor_tmp006_measure(&samples_temperature[count%SENSOR_NUM_SAMPLES]);
#endif
#ifdef MODULE_HDC1000
        int temp_hdc1000;
        sensor_hdc1000_measure(&temp_hdc1000,
                               &samples_humidity[count%SENSOR_NUM_SAMPLES]);
#endif
#ifndef BOARD_SAMR21_XPRO
        sensor_mq135_measure(&samples_airquality[count%SENSOR_NUM_SAMPLES]);
#endif
        count = (count+1)%SENSOR_NUM_SAMPLES;
        if (count == 0) {
            printf("[sensors] raw data T: %d, H: %d, A: %d\n",
                   sensor_get_temperature(),
                   sensor_get_humidity(),
                   sensor_get_airquality());
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
int sensor_start_thread(void)
{
    /* init sensors */
    if (sensor_init() != 0) {
        return -1;
    }
    /* start sensor thread for periodic measurements */
    return thread_create(sensor_thread_stack, sizeof(sensor_thread_stack),
                        THREAD_PRIORITY_MAIN, THREAD_CREATE_STACKTEST,
                        sensor_thread, NULL, "sensor_thread");
}
