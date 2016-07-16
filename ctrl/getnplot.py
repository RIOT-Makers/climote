#!/usr/bin/env python3

# coap stuff
from aiocoap import *
import asyncio
import time
# plot stuff
import matplotlib as mpl
#mpl.use("agg")
mpl.use('MacOSX')
import matplotlib.pyplot as plt
plt.ion()
import numpy
from matplotlib.ticker import FormatStrFormatter
from collections import deque

max_samples = 100
fig_samples = 2
sensor_ipv6 = "[fd49:88e0:1ed6:2:7982:4d5e:5728:2002]"
yFormatter = FormatStrFormatter('%.2f')
app_samples = False

@asyncio.coroutine
def main():
    protocol = yield from Context.create_client_context()
    samples = dict()
    samples['temperature'] = deque()
    samples['humidity'] = deque()
    samples['airquality'] = deque()
    if not app_samples:
        for i in range(0, max_samples):
            samples['temperature'].append(0)
            samples['humidity'].append(0)
            samples['airquality'].append(0)
        # end for
    # end if
    if (max_samples < 1) or (max_samples < fig_samples):
        return
    pos = 0
    while True:
        req_temperature = Message(code=GET)
        req_humidity = Message(code=GET)
        req_airquality = Message(code=GET)
        req_temperature.set_request_uri('coap://'+sensor_ipv6+'/temperature')
        req_humidity.set_request_uri('coap://'+sensor_ipv6+'/humidity')
        req_airquality.set_request_uri('coap://'+sensor_ipv6+'/airquality')
        try:
            res_temperature = yield from protocol.request(req_temperature).response
            res_humidity = yield from protocol.request(req_humidity).response
            res_airquality = yield from protocol.request(req_airquality).response
        except Exception as e:
            print('Failed to fetch resource:')
            print(e)
        else:
            t_temp = float(res_temperature.payload.decode('utf-8'))
            t_humi = float(res_humidity.payload.decode('utf-8'))
            t_airq = float(res_airquality.payload.decode('utf-8'))
            if not app_samples:
                samples['temperature'].popleft()
                samples['humidity'].popleft()
                samples['airquality'].popleft()
            # end if
            samples['temperature'].append(t_temp)
            samples['humidity'].append(t_humi)
            samples['airquality'].append(t_airq)
            print('Temperatur: %2.2f, Humitdy: %2.2f, AirQuality: %2.2f' %(t_temp, t_humi, t_airq))
        # end try
        pos = (pos + 1) % fig_samples
        if pos == 0:
            # init figure
            fig = plt.figure(figsize=(12,7))
            # add temperature
            ax = fig.add_subplot(311)
            ax.plot(samples['temperature'])
            ax.set_title('Sensor data')
            ax.yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
            ax.set_ylabel('Temperature')
            # add humidity
            ay = fig.add_subplot(312)
            ay.plot(samples['humidity'])
            ay.set_ylabel('Humidity')
            ay.yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
            # add pollution
            az = fig.add_subplot(313)
            az.plot(samples['airquality'])
            az.set_xlabel('samples [#]')
            az.set_ylabel('Pollution')
            az.yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
            # save figure
            #plt.savefig('sensordata.png')
            plt.pause(0.01)
            #plt.close(fig)
        time.sleep(0.5)

if __name__ == "__main__":
    asyncio.get_event_loop().run_until_complete(main())
