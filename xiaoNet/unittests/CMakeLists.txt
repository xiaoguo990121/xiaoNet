find_package(GTest REQUIRED)
add_executable(msgbuffer_unittest MsgBufferUnittest.cpp)
add_executable(inetaddress_unittest InetAddressUnittest.cpp)

set(UNITTEST_TARGETS
    msgbuffer_unittest
    inetaddress_unittest
)

set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_EXTENSIONS OFF)

include(GoogleTest)
foreach(T ${UNITTEST_TARGETS})
    target_link_libraries(${T} PRIVATE xiaoNet GTest::GTest)
endforeach()
