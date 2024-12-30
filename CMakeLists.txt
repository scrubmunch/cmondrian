cmake_minimum_required(VERSION 3.10)
project(cmondrian)

# Add termbox as a subdirectory
add_subdirectory(external/termbox_next)

# Your executable
add_executable(cmondrian src/main.cpp)

# Link with termbox
target_link_libraries(cmondrian PRIVATE termbox_next)

# Installation
install(TARGETS cmondrian DESTINATION bin)
