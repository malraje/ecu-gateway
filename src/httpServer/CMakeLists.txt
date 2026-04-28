# CMakeLists.txt in the httpServer folder

# Set the source files for the httpServer
set(HTTP_SERVER_SOURCE src/HttpServer.cpp)

# Add the library target
add_library(httpServer_lib ${HTTP_SERVER_SOURCE})

# Include directories
target_include_directories(
    httpServer_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/downloader/include
           ${CMAKE_SOURCE_DIR}/src/utils/include
)

target_link_libraries(
    httpServer_lib PRIVATE utils # Logger
                           Mbient::CloudGateway downloader_lib stopToken
)
