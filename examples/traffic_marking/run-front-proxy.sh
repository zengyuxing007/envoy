#!/bin/bash

if [ ! -f ./envoy-static ] ; then
    ln -s ./bazel-bin/source/exe/envoy-static envoy-static
fi

ulimit -c unlimited
./bazel-bin/source/exe/envoy-static -c ./front-envoy.yaml -l trace --base-id 1
