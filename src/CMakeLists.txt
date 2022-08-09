add_executable(upsampling_sample)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)
aux_source_directory(common COMMON_SRC)
add_library(common ${COMMON_SRC})

target_sources(upsampling_sample
PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/sample.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upsampling/upsampling.cpp
)

target_link_libraries(upsampling_sample
PRIVATE
    common
    ${OpenCV_LIBS}
)