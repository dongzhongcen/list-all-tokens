cmake_minimum_required(VERSION 3.21)
project(ListAllTokens VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

qt_add_executable(list-all-tokens
    src/main.cpp
)

target_link_libraries(list-all-tokens PRIVATE Qt6::Widgets Qt6::Sql)

if(WIN32)
    set_target_properties(list-all-tokens PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS list-all-tokens RUNTIME DESTINATION .)

set(CPACK_PACKAGE_NAME "list-all-tokens")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_GENERATOR "ZIP")
include(CPack)
