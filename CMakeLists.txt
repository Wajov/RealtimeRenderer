cmake_minimum_required(VERSION 3.20)

if (WIN32)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    add_compile_definitions(
        _USE_MATH_DEFINES
    )
endif()
add_compile_definitions(GLFW_INCLUDE_VULKAN)

project(RealtimeRenderer)

set(CMAKE_CXX_STANDARD 20)

include_directories(src)
file(GLOB_RECURSE SOURCE "src/*.cpp" "src/*.hpp")
add_executable(RealtimeRenderer ${SOURCE})

# Vulkan
find_package(Vulkan REQUIRED)

# GLFW
find_package(glfw3 REQUIRED)

# Eigen
find_package(Eigen3 REQUIRED)

include_directories(
    ${EIGEN3_INCLUDE_DIR}
)
target_link_libraries(
    RealtimeRenderer
    Vulkan::Vulkan
    glfw
)
