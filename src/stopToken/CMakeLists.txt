# Set the source files for the glibProxy library
set(SOURCE_FILES src/StopToken.cpp)

# Add the library target
add_library(stopToken STATIC ${SOURCE_FILES})

# Include directories
target_include_directories(stopToken PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# # Link against required libraries
target_link_libraries(stopToken PRIVATE utils # Logger
)
# target_link_libraries(glibProxy PRIVATE ${GLIBMM_LIBRARIES} ${GIOMM_LIBRARIES})
