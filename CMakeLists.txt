cmake_minimum_required(VERSION 3.15)
project(
    mtPool
    VERSION 1.0.0
    DESCRIPTION "Process-wide C++17 thread pool with optional delayed tasks"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

if(MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

option(MTPOOL_BUILD_SHARED "Build as shared library so EXE/DLLs share one pool" OFF)
option(MTPOOL_ENABLE_DELAYED "Build delayed-task scheduler (PostDelayedTask)" ON)
option(MTPOOL_BUILD_DEMO "Build delayed-task demo" ON)

set(MTPOOL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MtPool.cpp
)

if(MTPOOL_ENABLE_DELAYED)
    list(APPEND MTPOOL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sequence_token.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sequence_tracker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pending_task_queue.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/delayed_task_handle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/delayed_scheduler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/delayed_task_runner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/thread_pool_executor.cpp
    )
endif()

if(MTPOOL_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${MTPOOL_SOURCES})
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE MTPOOL_BUILDING
        PUBLIC  MTPOOL_SHARED
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
else()
    add_library(${PROJECT_NAME} STATIC ${MTPOOL_SOURCES})
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

if(MTPOOL_ENABLE_DELAYED)
    target_compile_definitions(${PROJECT_NAME} PUBLIC MTPOOL_ENABLE_DELAYED)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

if(MTPOOL_BUILD_DEMO AND MTPOOL_ENABLE_DELAYED)
    add_executable(mtpool_delayed_demo
        ${CMAKE_CURRENT_SOURCE_DIR}/demo/delayed_task_demo.cpp
    )
    target_link_libraries(mtpool_delayed_demo PRIVATE ${PROJECT_NAME})
endif()

install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${PROJECT_NAME}Targets"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/mtPool"
    DESTINATION include
)

foreach(_flag MTPOOL_ENABLE_DELAYED MTPOOL_BUILD_SHARED)
    if(${_flag})
        set(${_flag} ON)
    else()
        set(${_flag} OFF)
    endif()
endforeach()

install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/mtPoolConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
