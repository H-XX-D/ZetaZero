#!/bin/bash
curl -s --connect-timeout 5 http://192.168.0.165:9000/health > server_status.json 2>&1
if [ $? -eq 0 ]; then
    echo "Server is UP" >> server_status.json
else
    echo "Server is DOWN or Unreachable" >> server_status.json
fi
