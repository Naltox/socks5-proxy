cmake_minimum_required(VERSION 3.7)
project(socks5-proxy C)

set(CMAKE_C_STANDARD 99)

if(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_executable(c_playground main.c vector.h buff_utils.h socks5.c socks5.h io_events.h io_events_mac.c buffer.c buffer.h  net-connections.c net-connections.h buffer_chain.c buffer_chain.h)
elseif(UNIX AND NOT APPLE)
    add_executable(c_playground main.c vector.h buff_utils.h socks5.c socks5.h io_events.h io_events_linux.c buffer.c buffer.h net-connections.c net-connections.h buffer_chain.c buffer_chain.h)
endif()