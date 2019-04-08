## introduction

[./demo](./images/demo.jpg)
    
## build this example

docker-compose build
    
## launch this example
    
docker-compose up

## test 



### HTTP

``` shell
jesse@zengyuxing-ubuntu:~/serviceMesh/envoy-gf/examples/traffic_marking$ curl -H'x-envoy-prefer-cluster-color: green' 10.240.200.130:7001/service/1      

Hello from behind Envoy (service green)! hostname: cf84dad37c61 resolvedhostname: 172.24.0.4

jesse@zengyuxing-ubuntu:~/serviceMesh/envoy-gf/examples/traffic_marking$ 

jesse@zengyuxing-ubuntu:~/serviceMesh/envoy-gf/examples/traffic_marking$ curl -H'x-envoy-prefer-cluster-color: red' 10.240.200.130:7001/service/1     

Hello from behind Envoy (service red)! hostname: 8fbfe19a8e50 resolvedhostname: 172.24.0.2

jesse@zengyuxing-ubuntu:~/serviceMesh/envoy-gf/examples/traffic_marking$ 

jesse@zengyuxing-ubuntu:~/serviceMesh/envoy-gf/examples/traffic_marking$ curl -H'x-envoy-prefer-cluster-color: black' 10.240.200.130:7001/service/1    

Hello from behind Envoy (service simple)! hostname: 8c74a03dd769 resolvedhostname: 172.24.0.3
    
        
```

### TCP     

``` shell
jesse@zengyuxing-ubuntu:~/serviceMesh/envoy-gf/examples/traffic_marking$ curl -H'x-envoy-prefer-cluster-color: red' 10.240.200.130:7001/person_info/1

Person information: id: 1, firstName: jesse, lastName: zeng, age: 18,sex: MALE, income: 10000

jesse@zengyuxing-ubuntu:~/serviceMesh/envoy-gf/examples/traffic_marking$ 

jesse@zengyuxing-ubuntu:~/serviceMesh/envoy-gf/examples/traffic_marking$ curl -H'x-envoy-prefer-cluster-color: green' 10.240.200.130:7001/person_info/1   

Person information: id: 1, firstName: bill, lastName: gates, age: 18,sex: MALE, income: 88888888
    
```


should change the host (10.240.200.130) to your environment corresponding hostip


### explain

this demo example have a front and a python service (which have three color)

- service1 --> simple color
- service1-green --> green color
- service1-red  --> red color


the traffic mark logical is that , if the traffic have color , that request should be dispatch to which color service

if not have that color service,shoud be dispatch to the default service (have no color defined)




you can marking the entrance traffic using http header [ x-envoy-prefer-cluster-color ] 

``` shell
 curl -H 'x-envoy-prefer-cluster-color: any_string'
```

and the traffic_marking will be delivery to next service under the request service chain
