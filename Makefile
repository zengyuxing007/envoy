.PHONY: envoy 
envoy:
	bazel build -c dbg //source/exe:envoy
	cd /home/jesse/serviceMesh/envoy-gf
	cp -rf ./bazel-bin/source/exe/envoy-static envoy
build:envoy
	docker build -f Dockerfile-envoy -t vifoggy/envoy:test-latest .
