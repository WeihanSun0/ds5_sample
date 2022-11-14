add_executable(forSDS)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

target_sources(forSDS
PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/forSDS.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../playground.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../upsampling/upsampling.cpp
)

target_link_libraries(forSDS
PRIVATE
    ${OpenCV_LIBS}
)

add_executable(forSDS_test)
target_sources(forSDS_test
PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/data_test.cpp
)

target_link_libraries(forSDS_test
PRIVATE
    ${OpenCV_LIBS}
)
