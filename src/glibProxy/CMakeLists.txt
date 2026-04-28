# Set the source files for the glibProxy library
set(SOURCE_FILES src/GLibProxy.cpp)

# Add the library target
add_library(glibProxy STATIC ${SOURCE_FILES})

# Find and include necessary packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIBMM REQUIRED glibmm-2.4)
pkg_check_modules(GIOMM REQUIRED giomm-2.4)

# Include directories
target_include_directories(
    glibProxy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/interface ${GLIBMM_INCLUDE_DIRS}
                     ${GIOMM_INCLUDE_DIRS}
)

# Link against required libraries
target_link_libraries(
    glibProxy
    PRIVATE ${GLIBMM_LIBRARIES}
            ${GIOMM_LIBRARIES}
            stopToken
            utils # Logger
            gatewayServer_lib
            httpServer_lib
            Mbient::CloudGateway
)
