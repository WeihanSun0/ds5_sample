add_executable(upsampling_sample)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

target_sources(upsampling_sample
PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/sample.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upsampling/upsampling.cpp
)

target_link_libraries(upsampling_sample
PRIVATE
    ${OpenCV_LIBS}
)