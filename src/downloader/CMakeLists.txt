# CMakeLists.txt in the downloader folder

find_package(MbientCloudGateway REQUIRED)
find_package(MbientCore REQUIRED)

# Set the source files for the downloader library
set(DOWNLOADER_SOURCE src/CGWDownloader.cpp src/DownloadUtils.cpp)

# Add the library target
add_library(downloader_lib ${DOWNLOADER_SOURCE})

# Include directories
target_include_directories(
    downloader_lib
    PUBLIC ${CMAKE_SOURCE_DIR}/src/utils/include ${CMAKE_CURRENT_SOURCE_DIR}/include
           ${CMAKE_SOURCE_DIR}/src/gatewayServer/include ${CMAKE_SOURCE_DIR}/src/httpServer/include
)

target_link_libraries(
    downloader_lib PRIVATE ${Boost_LIBRARIES} utils Mbient::CloudGateway httpServer_lib
                           gatewayServer_lib
)
