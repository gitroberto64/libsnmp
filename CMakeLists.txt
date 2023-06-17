cmake_minimum_required(VERSION 3.0.0)
project(snmp VERSION 0.1.0)


add_library(${PROJECT_NAME} STATIC snmp.cpp)

add_executable(agent_test agent_test.cpp)
target_link_libraries(agent_test snmp)

add_executable(manager_test manager_test.cpp)
target_link_libraries(manager_test snmp)

