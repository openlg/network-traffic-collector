#!/bin/bash

NTC="/opt/ntc"

if [ ! -f "$NTC" ]; then
  echo "ntc not found in path $NTC"
  exit 1;
fi

pid=$(ps aux | grep '/opt/ntc' | grep -v grep | awk '{print $2}')

if [ -n "$pid" ]; then
    echo "kill exist ntc process $pid"
    kill -2 "$pid"
    sleep 2
fi

if [ ! -x "$NTC" ]; then
  chmod +x "$NTC"
fi

/opt/ntc -I10s -s https://cloud.tapdata.net/api/tcm/agent/network_traffic >> /opt/ntc.log 2>&1 &

pid=$(ps aux | grep '/opt/ntc' | grep -v grep | awk '{print $2}')
echo "ntc started, pid is $pid"
