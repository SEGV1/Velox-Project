set(Velox_interceptor_SRCS
  interceptor.c
  fdchanmap.c
  fdpool.c
  req-resp-channel.c
  )

add_library(velox_interceptor SHARED
  ${Velox_interceptor_SRCS})

target_link_libraries(velox_interceptor
  velox_rpc_c
  protobuf-c
)
