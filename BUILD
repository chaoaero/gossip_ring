proto_library(
    name = 'gossip_ring_proto',
    srcs = [
        'gossip_ring.proto',
    ],
    deps = [
        '//poppy:rpc_option_proto',
    ]
)

cc_library(
    name = 'failure_detector',
    srcs = [
        'failure_detector.cc',
        'arrival_window.cc',
    ],
    deps = [
        '//thirdparty/glog:glog',
    ]
)


cc_binary(
    name = 'gossip_server',
    srcs = [
        'gossip_server.cc',
        'gossip_server_impl.cc'
    ],
    deps = [
        ':gossip_ring_proto',
        ':failure_detector',
        '//common/base/string:string',
        '//poppy:poppy',
        '//thirdparty/gflags:gflags',
        '//thirdparty/glog:glog',
        '//common/system/concurrency:concurrency',
        '//common/system/concurrency:sync_object',
        '//common/system/net:ip_address',
        '//common/system/time:time',
        '//common/system/timer:timer',
        #'//ext/thirdparty/hiredis:hiredis',
    ],
)

cc_binary(
    name = 'client',
    srcs = [
        'client.cc',
    ],
    deps = [
        ':gossip_ring_proto',
        '//common/base/string:string',
        '//poppy:poppy',
        '//thirdparty/gflags:gflags',
        '//thirdparty/glog:glog',
    ]
)
