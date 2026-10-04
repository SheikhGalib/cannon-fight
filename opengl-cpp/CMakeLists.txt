cmake_minimum_required(VERSION 3.15)
project(opengl_app CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEPS ${CMAKE_SOURCE_DIR}/deps)

# --- GLAD (OpenGL loader) ---
add_library(glad STATIC ${DEPS}/glad/src/glad.c)
target_include_directories(glad PUBLIC ${DEPS}/glad/include)

# --- GLFW (precompiled MinGW-w64 binaries) ---
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION ${DEPS}/glfw/lib-mingw-w64/libglfw3.a
    INTERFACE_INCLUDE_DIRECTORIES ${DEPS}/glfw/include
)

# --- GLM (header-only) ---
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${DEPS})

# --- App ---
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE glad glfw glm opengl32 gdi32)
target_link_options(app PRIVATE -static-libgcc -static-libstdc++ -static)
