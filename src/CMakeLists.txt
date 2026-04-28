# CMakeLists.txt in the src folder
set(TARGET_NAME ${PROJECT_NAME})

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Include directories for all targets
include_directories(${GLIB_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src/utils/include)

# Link directories for all targets
link_directories(${GLIB_LIBRARY_DIRS})

add_subdirectory(stopToken)
add_subdirectory(glibProxy)
add_subdirectory(downloader)
add_subdirectory(gatewayServer)
add_subdirectory(httpServer)
add_subdirectory(utils)

set(ECUG_COMMON_LIBS
    glibProxy
    httpServer_lib
    downloader_lib
    gatewayServer_lib
    stopToken
    -pthread
    OpenSSL::Crypto
    cpprestsdk::cpprest
    Boost::system
    -lsystemd
    Mbient::CloudGateway
)

# add exe
add_executable(${TARGET_NAME} "${CMAKE_CURRENT_LIST_DIR}/main.cpp")

target_link_libraries(${TARGET_NAME} PRIVATE ${ECUG_COMMON_LIBS} ${SYSTEMD_LIBRARIES})
