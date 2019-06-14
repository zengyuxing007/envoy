.PHONY: envoy 
envoy:
	bazel build -c dbg //source/exe:envoy
	cd /home/jesse/serviceMesh/envoy-gf
	cp -rf ./bazel-bin/source/exe/envoy-static envoy
release:
	bazel build //source/exe:envoy
	cd /home/jesse/serviceMesh/envoy-gf
	cp -rf ./bazel-bin/source/exe/envoy-static envoy-release

build:envoy
	docker build -f Dockerfile-envoy -t vifoggy/envoy:test-latest .
build-yx:envoy
	docker build -f Dockerfile-envoy-yx -t vifoggy/envoy:yx-latest .
build-yx-release: envoy-release
	docker build -f Dockerfile-envoy-yx-release -t vifoggy/envoy:yx-release-${shell date +"%Y%m%d%H%M%S"} .

dockerpush: build
	docker push vifoggy/envoy:test-latest
push-yx: build-yx
	docker push vifoggy/envoy:yx-latest
go-code:
	protoc --proto_path=/home/jesse/go/src/github.com/gogo/protobuf/:/home/jesse/go/src/github.com/envoyproxy/protoc-gen-validate:/home/jesse/go/src/github.com/googleapis/googleapis:./api --go_out=./api-go-code api/envoy/config/filter/network/http_connection_manager/v2/http_connection_manager.proto 
