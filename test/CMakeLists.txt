add_executable(minheap_test ds/minheap_test.cpp)
target_link_libraries(minheap_test sznet pthread)

add_executable(timerqueue_unittest timer/timerqueue_unittest.cpp)
target_link_libraries(timerqueue_unittest sznet pthread)

add_executable(tcp_echo_client echo/tcp_echo/tcp_echo_client.cpp)
target_link_libraries(tcp_echo_client sznet pthread)

add_executable(tcp_echo_server echo/tcp_echo/tcp_echo_server.cpp)
target_link_libraries(tcp_echo_server sznet pthread)