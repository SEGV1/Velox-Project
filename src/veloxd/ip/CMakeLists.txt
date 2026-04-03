set(veloxd_ip_SRCS
  Ip.cpp
  IpHdr.cpp
  reassembly/Reassembler.cpp
  reassembly/Fragmenter.cpp
  )

add_library(veloxd_ip
  ${veloxd_ip_SRCS}
  )
