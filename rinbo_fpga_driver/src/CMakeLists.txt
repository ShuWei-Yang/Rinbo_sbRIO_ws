find_library(GRPC_PROTO_LIB grpc_proto_lib)
if (NOT GRPC_PROTO_LIB)
    message(FATAL_ERROR "Not found GRPC_PROTO Library.")
endif()

find_package(Protobuf REQUIRED)
set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/grpc_core/robot_protos)

set(PROTO_FILES 
    ${PROTO_SRC_DIR}/Motor.proto 
    ${PROTO_SRC_DIR}/Power.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PROTO_SRC_DIR})

include_directories(${CMAKE_SOURCE_DIR}/include)

set(YAML_CPP_INCLUDE_DIR $ENV{HOME}/rinbo_sbRIO_ws/install/include)
include_directories(${YAML_CPP_INCLUDE_DIR})

add_executable(fpga_driver 
    fpga_server.cpp
    console.cpp    
    color.cpp
    fpga_handler.cpp
    log.cpp
    NiFpga.c
    ${PROTO_SRCS} 
    ${PROTO_HDRS} 
)

find_package(Eigen3 REQUIRED)

set(YAML_CPP_LIBRARIES $ENV{HOME}/rinbo_sbRIO_ws/install/lib/libyaml-cpp.a)

target_link_libraries(fpga_driver 
    ${YAML_CPP_LIBRARIES} 
    Eigen3::Eigen     
    protobuf::libprotobuf  
    ${GRPC_PROTO_LIB}   
    ${CURSES_LIBRARY}  
    ncurses          
    ${CMAKE_DL_LIBS}     
    ${_CORE_LIBRARIES}     
)