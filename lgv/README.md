## local setup with RIOT native

1. start RIOT
    - cd /Volumes/workspace/github/riot-makers/climote/monica
    - make clean all term
2. setup bridge
    - sudo ifconfig bridge42 addm tap0
    - sudo ifconfig bridge42 inet6 fd17:cafe:cafe:3::1/64
3. run mqtt broker
    - cd /Volumes/workspace/github/mosquitto.rsmb/rsmb/src
    - ./broker_mqtts config.conf
4. run MQTT.fx
    - subscribe monica/info and monica/climate
5. setup RIOT and trigger mqtt
    - ifconfig 6 add fd17:cafe:cafe:3::3/64
    - btn <- enable mqtt
    - btn <- trigger publish
6. use CoAP
    - open firefox
    - configure NON-CON, disable retrans and dups, display unknown, neg block later
    - goto coap://[fd17:cafe:cafe:3::3]:5683/.well-known/core

## global setup

### riot nodes

1. OSX setup network and routes
    - sudo route add -inet6 -net fd17:cafe:cafe::/48 fd17:cafe:cafe:2::1
2. connect to pi
    - ssh pi@fd17:cafe:cafe:2::1
    - screen
    - sudo radvd -m stderr -d 5 -n
    - cd /opt/src/mosquitto.rsmb/rsmb/src
    - ./broker_mqtts config.conf
    - ping6 fd17:cafe:cafe:3:d1c1:6d6b:ab6a:1336
3. enable nodes:
    - press button first time -> enable mqtt
    - press button second time -> trigger publish

4. run wireshark on OSX
5. test mqtt and coap
    - mosquitto_sub -h fd17:cafe:cafe:2::1 -p 1886 -t monica/info -t monica/climate

- prefix fd17:cafe:cafe:3::/64
- alice: fd17:cafe:cafe:3:d1c1:6d6b:ab6a:1336
- bob: fd17:cafe:cafe:3:d1c1:6d7f:ab01:1336
CoAP alice
- coap-client -m get -N coap://[fd17:cafe:cafe:3:d1c1:6d6b:ab6a:1336]/monica/info
- coap-client -m get -N coap://[fd17:cafe:cafe:3:d1c1:6d6b:ab6a:1336]/monica/climate
CoAP bob
- coap-client -m get -N coap://[fd17:cafe:cafe:3:d1c1:6d7f:ab01:1336]/monica/info
- coap-client -m get -N coap://[fd17:cafe:cafe:3:d1c1:6d7f:ab01:1336]/monica/climate
