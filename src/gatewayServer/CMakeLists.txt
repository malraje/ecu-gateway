# CMakeLists.txt in the gatewayServer folder

# Set the source files for the gatewayServer executable
set(GATEWAY_SERVER_SOURCE src/Server.cpp)

# Add the library target
add_library(gatewayServer_lib ${GATEWAY_SERVER_SOURCE})

# Include directories
target_include_directories(
    gatewayServer_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/utils/include
           ${CMAKE_SOURCE_DIR}/src/downloader/include
           ${GLIB_INCLUDE_DIRS} # Include GLib include directories found by pkg-config
)

target_link_libraries(
    gatewayServer_lib PRIVATE utils # Logger
                              Mbient::CloudGateway ${GLIB_LIBRARIES} downloader_lib stopToken
)
