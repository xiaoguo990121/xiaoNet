cmake_minimum_required(VERSION 3.10)
project(xiaoNet)

option(BUILD_SHARED_LIBS "Build xiaoNet as a shared lib" OFF)
option(BUILD_TESTING "Build tests" OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules/)

set(XIAONET_MAJOR_VERSION 0)
set(XIAONET_MINOR_VERSION 0)
set(XIAONET_PATCH_VERSION 1)
set(XIAONET_VERSION ${XIAONET_MAJOR_VERSION}.${XIAONET_MINOR_VERSION}.${XIAONET_PATCH_VERSION})

include(GNUInstallDirs)
set(INSTALL_BIN_DIR 
    ${CMAKE_INSTALL_BINDIR}
    CACHE PATH "Installation directory for binaries")
set(INSTALL_LIB_DIR
    ${CMAKE_INSTALL_LIBDIR}
    CACHE PATH "Installation directory for libraries")
set(INSTALL_INCLUDE_DIR
    ${CMAKE_INSTALL_INCLUDEDIR}
    CACHE PATH "Installation directory for header files")
set(DEF_INSTALL_XIAONET_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/XiaoNet)
set(INSTALL_XIAONET_CMAKE_DIR 
    ${DEF_INSTALL_XIAOLOG_CMAKE_DIR}
    CACHE PATH "Installation directory for cmake files")


add_library(${PROJECT_NAME})
if(BUILD_SHARED_LIBS)
    list(
        FIND
        CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
        "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIB_DIR}"
        isSystemDir
    )

    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIB_DIR}")
    endif("${isSystemDir}" STREQUAL "-1")

    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES VERSION
                    ${XIAOLOG_VERSION}
                    SOVERSION
                    ${XIAOLOG_MAJOR_VERSION}   
    )
endif(BUILD_SHARED_LIBS)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/exports/xiaoNet/exports.h)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME XiaoLog)


set(XIAONET_INCLUDE)


set(public_net_headers
    xiaoNet/net/AsyncStream.h
    xiaoNet/net/Channel.h
)

set(XIAONET_SOURCES
    xiaoNet/net/Channel.cpp
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}> 
           $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/exports>
)
set(PROJECT_BASE_PATH ${PROJECT_SOURCE_DIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
    find_package(GTest)
    if(GTest_FOUND)
        enable_testing()
        add_subdirectory(unittests)
    endif()
endif()

target_sources(
    ${PROJECT_NAME}
    PRIVATE ${XIAONET_SOURCES}
            ${XIAONET_INCLUDE}
            ${CMAKE_CURRENT_BINARY_DIR}/exports/xiaoNet/exports.h)


find_package(XiaoLog)
target_link_libraries(${PROJECT_NAME} PRIVATE XiaoLog::XiaoLog)