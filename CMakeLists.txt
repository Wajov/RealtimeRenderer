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

# Compile shaders to SPIR-V
find_program(GLSLANG_VALIDATOR NAMES glslangValidator REQUIRED)
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shader)
set(SHADER_BINARY_DIR ${CMAKE_SOURCE_DIR}/shader)
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})
file(GLOB SHADER_FILES ${SHADER_SOURCE_DIR}/*.vert ${SHADER_SOURCE_DIR}/*.frag)
set(SPIRV_FILES "")
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER_FILE} NAME)
    set(SPV_FILE ${SHADER_BINARY_DIR}/${FILE_NAME}.spv)
    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_FILE} -o ${SPV_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling ${FILE_NAME} to SPIR-V"
        VERBATIM
    )
    list(APPEND SPIRV_FILES ${SPV_FILE})
endforeach()
add_custom_target(CompileShaders ALL DEPENDS ${SPIRV_FILES})
add_dependencies(${PROJECT_NAME} CompileShaders)

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
