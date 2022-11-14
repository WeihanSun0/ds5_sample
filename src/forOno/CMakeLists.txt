add_executable(forOno)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

target_sources(forOno
PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/forOno.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../upsampling/upsampling.cpp
)

target_link_libraries(forOno
PRIVATE
    ${OpenCV_LIBS}
)


add_executable(pcViewer)


target_sources(pcViewer
PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/pc_viewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../viewer.hpp
)

target_link_libraries(pcViewer
PRIVATE
    ${OpenCV_LIBS}
)