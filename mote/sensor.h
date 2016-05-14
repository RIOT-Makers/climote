/**
 * @ingroup     climote
 * @{
 *
 * @file
 * @brief       Defines sensor stuff
 *
 * @author      smlng <s@mlng.net>
 *
 */

#ifndef SENSOR_H_
#define SENSOR_H_

int sensor_get_airquality(void);
int sensor_get_humidity(void);
int sensor_get_temperature(void);
int sensor_start_thread(void);

#endif // SENSOR_H_
/** @} */
