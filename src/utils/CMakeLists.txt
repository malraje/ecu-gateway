set(TARGET_NAME utils)

add_library(utils STATIC src/Logger.cpp src/LogInitializer.cpp)

target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${Boost_INCLUDE_DIR})

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${TARGET_NAME} PRIVATE log4cplus::log4cplus dltappender::dltappender)
