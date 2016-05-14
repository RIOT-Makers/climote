#!/bin/bash
DATE=$(date +%s)
COAPCLIENT=$(which coap-client)
[ -z  "!$COAPCLIENT"] && {
    echo "coap-client not found, install libcoap!"
    exit 1
}
# this is a workaround for non-Linux systems, i.e., MacOS X and BSD UNIX
READLINK=$(which greadlink)
[ -z "$READLINK" ] && {
	READLINK=$(which readlink)
}
GREP=$(which ggrep)
[ -z "$GREP" ] && {
	GREP=$(which grep)
}
SCRIPT_DIR=$(dirname "$($READLINK -f "$0")")
SENSORS="$SCRIPT_DIR/sensors.txt"
NODES="$SCRIPT_DIR/nodes.txt"
TIMEOUT="10"
for SENSOR in `cat $SENSORS`; do
    for NODE in `cat $NODES`; do
        VAL=$($COAPCLIENT -m GET -B $TIMEOUT coap://[$NODE]/$SENSOR | $GREP -v "^v")
        [ -z "$VAL" ] && { VAL="NA"; }
        echo "$DATE $NODE $SENSOR $VAL"
    done
done
