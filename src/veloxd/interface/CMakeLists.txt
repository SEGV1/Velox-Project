set(veloxd_interface_SRCS
  Interface.cpp
  PhysIface.cpp
  TunIface.cpp
  )

add_library(veloxd_interface
  ${veloxd_interface_SRCS}
  )
