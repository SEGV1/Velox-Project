set(veloxd_base_SRCS
  ShellOp.cpp
  Csum16.cpp
  RawMutex.cpp
  )

add_library(veloxd_base
  ${veloxd_base_SRCS}
  )
