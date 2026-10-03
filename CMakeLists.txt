cmake_minimum_required(VERSION 4.3)

project(Learning_Vulkan CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Option to toggle C++20 Modules
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan" OFF)

if (ENABLE_CPP20_MODULE)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
else ()
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif ()

# 2. Dependencies
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan 1.4 REQUIRED)

# 3. Setup Vulkan Module or Fallback Interface
if (ENABLE_CPP20_MODULE)
    add_library(VulkanCppModule)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule PUBLIC
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
            VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
    target_include_directories(VulkanCppModule PUBLIC "${Vulkan_INCLUDE_DIR}")
    target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)

    target_sources(VulkanCppModule
            PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES
            BASE_DIRS "${Vulkan_INCLUDE_DIR}"
            FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )
else ()
    # Dummy interface when modules are disabled
    add_library(VulkanCppModule INTERFACE)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)
    target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
    target_compile_definitions(VulkanCppModule INTERFACE
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
            VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
endif ()

# 4. Create Main Executable
add_executable(Learning_Vulkan src/main.cpp)

set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/Learning_Vulkan_App")
set_target_properties(Learning_Vulkan PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

target_link_libraries(Learning_Vulkan Vulkan::cppm glfw)
target_compile_definitions(Learning_Vulkan PRIVATE VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS)

# Tell the C++ code whether to use imports or includes
if (ENABLE_CPP20_MODULE)
    target_compile_definitions(Learning_Vulkan PRIVATE USE_CPP20_MODULES=1)
endif ()

# 5. Compile Slang Shader to SPIR-V
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
set(SHADERS_DIR "${OUTPUT_DIR}/shaders")

# Rebuild when any .slang file changes, including imported modules
file(GLOB SLANG_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.slang")

add_custom_command(
        OUTPUT "${SHADERS_DIR}/slang.spv"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADERS_DIR}"
        COMMAND ${SLANGC_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang"
        -target spirv -profile spirv_1_4 -emit-spirv-directly
        -fvk-use-entrypoint-name -entry vertMain -entry fragMain
        -o "${SHADERS_DIR}/slang.spv"
        DEPENDS ${SLANG_SOURCES}
        COMMENT "Compiling Slang Shader"
        VERBATIM
)

add_custom_target(CompileShaders DEPENDS "${SHADERS_DIR}/slang.spv")
add_dependencies(Learning_Vulkan CompileShaders)
