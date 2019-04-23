.PHONY: envoy 
envoy:
	bazel build -c dbg //source/exe:envoy
	cd /home/jesse/serviceMesh/envoy-gf
	cp -rf ./bazel-bin/source/exe/envoy-static envoy
build:envoy
	docker build -f Dockerfile-envoy -t vifoggy/envoy:test-latest .
build-yx:envoy
	docker build -f Dockerfile-envoy-yx -t vifoggy/envoy:yx-latest .

dockerpush: build
	docker push vifoggy/envoy:test-latest
push-yx: build-yx
	docker push vifoggy/envoy:yx-latest
