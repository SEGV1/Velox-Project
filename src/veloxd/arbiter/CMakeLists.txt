set(veloxd_arbiter_SRCS
  Arbiter.cpp
  RpcDispatch.cpp
  Endpoint.cpp

  reactor/IoHandle.cpp
  reactor/EventReactor.cpp
  reactor/SysPoller.cpp

  agent/AgentHub.cpp
  agent/AgentChannel.cpp
  )

add_library(veloxd_arbiter
  ${veloxd_arbiter_SRCS}
  )
