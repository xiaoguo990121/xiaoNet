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
    ${DEF_INSTALL_XIAONET_CMAKE_DIR}
    CACHE PATH "Installation directory for cmake files")
message("${INSTALL_XIAONET_CMAKE_DIR}")

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


set(XIAONET_SOURCES
    xiaoNet/utils/ConcurrentTaskQueue.cpp
    xiaoNet/utils/MsgBuffer.cpp
    xiaoNet/utils/SerialTaskQueue.cpp
    xiaoNet/utils/TimingWheel.cpp
    xiaoNet/utils/Utilities.cpp
    xiaoNet/net/EventLoop.cpp
    xiaoNet/net/EventLoopThread.cpp
    xiaoNet/net/EventLoopThreadPool.cpp
    xiaoNet/net/InetAddress.cpp
    xiaoNet/net/TcpClient.cpp
    xiaoNet/net/TcpServer.cpp
    xiaoNet/net/Channel.cpp
    xiaoNet/net/inner/Acceptor.cpp
    xiaoNet/net/inner/Connector.cpp
    xiaoNet/net/inner/Poller.cpp
    xiaoNet/net/inner/Socket.cpp
    xiaoNet/net/inner/MemBufferNode.cpp
    xiaoNet/net/inner/StreamBufferNode.cpp
    xiaoNet/net/inner/AsyncStreamBufferNode.cpp
    xiaoNet/net/inner/TcpConnectionImpl.cpp
    xiaoNet/net/inner/Timer.cpp
    xiaoNet/net/inner/TimerQueue.cpp
    xiaoNet/net/inner/poller/EpollPoller.cpp
)

set(private_headers
    xiaoNet/net/inner/Acceptor.h
    xiaoNet/net/inner/Connector.h
    xiaoNet/net/inner/Poller.h
    xiaoNet/net/inner/Socket.h
    xiaoNet/net/inner/TcpConnectionImpl.h
    xiaoNet/net/inner/Timer.h
    xiaoNet/net/inner/TimerQueue.h
    xiaoNet/net/inner/poller/EpollPoller.h
)

set(public_net_headers
    xiaoNet/net/EventLoop.h
    xiaoNet/net/EventLoopThread.h
    xiaoNet/net/EventLoopThreadPool.h
    xiaoNet/net/InetAddress.h
    xiaoNet/net/TcpClient.h
    xiaoNet/net/TcpConnection.h
    xiaoNet/net/TcpServer.h
    xiaoNet/net/AsyncStream.h
    xiaoNet/net/callbacks.h
#   xiaoNet/net/Resolver.h
    xiaoNet/net/Channel.h
    xiaoNet/net/Certificate.h
    xiaoNet/net/TLSPolicy.h
)

set(public_utils_headers
    xiaoNet/utils/ConcurrentTaskQueue.h
    xiaoNet/utils/LockFreeQueue.h
    xiaoNet/utils/MsgBuffer.h
    xiaoNet/utils/NonCopyable.h
#   xiaoNet/utils/ObjectPoll.h
    xiaoNet/utils/SerialTaskQueue.h
    xiaoNet/utils/TaskQueue.h
    xiaoNet/utils/TimingWheel.h
    xiaoNet/utils/Utilities.h
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}> $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/exports>
    PRIVATE ${PROJECT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/xiaoNet/utils
            ${PROJECT_SOURCE_DIR}/xiaoNet/net
            ${PROJECT_SOURCE_DIR}/xiaoNet/net/inner
)
set(PROJECT_BASE_PATH ${PROJECT_SOURCE_DIR})

if(BUILD_TESTING)
    add_subdirectory(xiaoNet/tests)
    find_package(GTest)
    if(GTest_FOUND)
        enable_testing()
        add_subdirectory(xiaoNet/unittests)
    endif()
endif()

if(WIN32)
else(WIN32)
    set(XIAONET_SOURCES ${XIAONET_SOURCES} xiaoNet/net/inner/FileBufferNodeUnix.cpp)
endif(WIN32)

target_sources(
    ${PROJECT_NAME}
    PRIVATE ${XIAONET_SOURCES}
            ${public_net_headers}
            ${public_utils_headers}
            ${private_headers}
            ${CMAKE_CURRENT_BINARY_DIR}/exports/xiaoNet/exports.h)


find_package(XiaoLog)
target_link_libraries(${PROJECT_NAME} PRIVATE XiaoLog::XiaoLog)

install(
    TARGETS xiaoNet
    EXPORT XiaoNetTargets
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/exports/xiaoNet/exports.h DESTINATION ${INSTALL_INCLUDE_DIR}/xiaoNet)
install(FILES ${public_net_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/xiaoNet)
install(FILES ${public_utils_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/xiaoNet)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/templates/XiaoNetConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/XiaoNetConfig.cmake
    INSTALL_DESTINATION ${INSTALL_XIAONET_CMAKE_DIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/XiaoNetConfigVersion.cmake
    VERSION ${XIAONET_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/XiaoNetConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/XiaoNetConfigVersion.cmake"
    DESTINATION "${INSTALL_XIAONET_CMAKE_DIR}"
    COMPONENT dev
)

install(
    EXPORT XiaoNetTargets
    DESTINATION "${INSTALL_XIAONET_CMAKE_DIR}"
    NAMESPACE XiaoNet::
    COMPONENT dev
)
