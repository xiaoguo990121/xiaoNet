add_executable(timer_test TimerTest.cpp)
add_executable(timing_wheel_test TimingWheelTest.cpp)
add_executable(tcp_client_test TcpClientTest.cpp)
add_executable(tcp_server_test TcpServerTest.cpp)

set(targets_list
    timer_test
    timing_wheel_test
    tcp_client_test
    tcp_server_test
)

set_property(TARGET ${targets_list} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${targets_list} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${targets_list} PROPERTY CXX_EXTENSIONS OFF)

foreach(T ${targets_list})
    target_link_libraries(${T} PRIVATE xiaoNet)
endforeach()
