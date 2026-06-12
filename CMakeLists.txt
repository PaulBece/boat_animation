project( 14_Boat_glfw )

message( "Source of application:  ${CMAKE_SOURCE_DIR} " )
message( "Subproject name:  ${PROJECT_NAME} " )

set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/bin")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR} )

# Enable C++17 Standard Execution Profiles
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_BUILD_TYPE Release )

link_libraries(glfw)

include_directories("${GLFW_SOURCE_DIR}/deps")

if (MATH_LIBRARY)
    message(${MATH_LIBRARY})
    link_libraries("${MATH_LIBRARY}")
endif()

# Workaround for the MS CRT deprecating parts of the standard library
if (MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if (WIN32)
    set(ICON ../glfw.rc)
elseif (APPLE)
    set(ICON ../glfw.icns)
endif()

set(GLAD_GL "${GLFW_SOURCE_DIR}/deps/glad/gl.h")
set(GLAD_GLES2 "${GLFW_SOURCE_DIR}/deps/glad/gles2.h")
set(GETOPT "${GLFW_SOURCE_DIR}/deps/getopt.h"
           "${GLFW_SOURCE_DIR}/deps/getopt.c")
set(TINYCTHREAD "${GLFW_SOURCE_DIR}/deps/tinycthread.h"
                "${GLFW_SOURCE_DIR}/deps/tinycthread.c")

# Locate Desktop OpenGL Driver Context Implementations
find_package(OpenGL REQUIRED)

# RESTORED: Your original working GLAD logic pathing
file(GLOB SOURCES "*.cpp" ${DEPENDENCY_DIR}/include/glad/glad/glad.c )
file(GLOB HEADERS "*.h" )
file(GLOB SHADERS "shaders/*.vert" "shaders/*.frag" "shaders/*.vs" "shaders/*.fs" )

# Assemble Executable Targeting Modern Graphics Pipeline Configurations
add_executable(
 ${PROJECT_NAME} WIN32 MACOSX_BUNDLE  
 ${HEADERS} ${SOURCES} ${SHADERS}
 ${ICON} ${GLAD_GL}   )

# Inject asset tracking directories straight to our compiler preprocessor definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    SHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/shaders/"
    MODELS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/models/"
)

message( "\nBuild mode test is: ${CMAKE_BUILD_TYPE}" )

set(GUI_ONLY_BINARIES  ${PROJECT_NAME})

message( "\nBuild C++ standard is: ${CMAKE_CXX_STANDARD}" ) 

SET(SUBSYSTEM_LINK_FLAGS "-mconsole -mwindows")

target_link_libraries(  ${PROJECT_NAME} 
                        ${SUBSYSTEM_LINK_FLAGS}
                        )