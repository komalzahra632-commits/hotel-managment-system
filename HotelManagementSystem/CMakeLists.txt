cmake_minimum_required(VERSION 3.16)
project(HotelManagementSystem LANGUAGES CXX)

add_executable(HotelManagementSystem
    main.cpp
    HotelSystem.cpp
)

target_compile_features(HotelManagementSystem PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(HotelManagementSystem PRIVATE /W4 /permissive-)
else()
    target_compile_options(HotelManagementSystem PRIVATE -Wall -Wextra -Wpedantic)
endif()
