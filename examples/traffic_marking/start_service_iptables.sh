#!/bin/sh

/usr/local/bin/istio-iptables.sh -p 18081 -u 1000  -m REDIRECT -i "*" -x ""   -b "" -d "9080"
python3 /code/service.py &
su - demo_user -s /bin/bash -c "envoy -c /etc/service-envoy.yaml --service-cluster service${SERVICE_NAME} -l trace"
