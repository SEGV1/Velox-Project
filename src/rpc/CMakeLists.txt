# # 1. Delete previously generated C/C++ code by protoc

# #    For convenience, cmake execute_process is used to invoke protoc
# #      to generate C/C++ code. This causes target {rpc_c, rpc_cpp}
# #      to become out-of-date, thereby triggering recompilation.
# #    To avoid unnecessary generation and recompilation,
# #      comment out steps 1-3 below once the protobuf definition files
# #      are no longer changing

# execute_process(
#   COMMAND cmake -E remove_directory "${CMAKE_CURRENT_SOURCE_DIR}/c_out"
#   COMMAND cmake -E remove_directory "${CMAKE_CURRENT_SOURCE_DIR}/cpp_out"
#   )
# execute_process(
#   COMMAND cmake -E make_directory
#       "${CMAKE_CURRENT_SOURCE_DIR}/c_out"
#       "${CMAKE_CURRENT_SOURCE_DIR}/cpp_out"
#   )

# # 2. Glob the protobuf .proto files

# # file(GLOB rpc_proto_SRCS
# #   LIST_DIRECTORIES true
# #   ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto
# #   )

# set(rpc_proto_SRCS
#   ${CMAKE_CURRENT_SOURCE_DIR}/proto/request.proto
#   ${CMAKE_CURRENT_SOURCE_DIR}/proto/response.proto
#   )

# # 3. Invoke protoc to generate C/C++ code

# foreach(pfile IN LISTS rpc_proto_SRCS)
#   execute_process(
#     COMMAND protoc-c
#         -I${CMAKE_CURRENT_SOURCE_DIR}/proto
#         --c_out=${CMAKE_CURRENT_SOURCE_DIR}/c_out
#         ${pfile}
#     COMMAND protoc
#         -I${CMAKE_CURRENT_SOURCE_DIR}/proto
#         --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/cpp_out
#         ${pfile}
#   )
# endforeach()
