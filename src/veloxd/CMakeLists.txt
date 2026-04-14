set(Veloxd_SRCS
  CfgParser.cpp
  Logging.cpp
  main.cpp
  Veloxd.cpp
  Session.cpp
  )

add_executable(Veloxd "${Veloxd_SRCS}")
set_target_properties(Veloxd
  PROPERTIES  OUTPUT_NAME veloxd
  )

add_subdirectory(base)
add_subdirectory(interface)
add_subdirectory(ip)
# add_subdirectory(icmp)
add_subdirectory(tcp)
add_subdirectory(udp)
add_subdirectory(arbiter)
target_link_libraries(Veloxd
  veloxd_arbiter
  veloxd_tcp
  veloxd_udp
  # veloxd_icmp
  veloxd_ip
  veloxd_interface
  veloxd_base
  velox_rpc_cpp
  Boost::program_options
  Boost::log
  protobuf::libprotobuf
  ev
  )
