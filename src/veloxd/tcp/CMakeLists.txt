set(veloxd_tcp_SRCS
  Tcp.cpp
  TcpSession.cpp
  )

add_library(veloxd_tcp
  ${veloxd_tcp_SRCS}
  )
