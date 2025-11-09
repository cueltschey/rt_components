
include_directories(hdr)
file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(uuagent ${SRC_FILES})

target_link_libraries(uuagent ${UHD_LIBRARIES} ${YAML_CPP_LIBRARIES} ${ZEROMQ_LIBRARIES} Boost::program_options)
