cmake_minimum_required(VERSION 3.10)
project(EventDrivenTradingEngine)
set(CMAKE_CXX_STANDARD 23)

add_executable(engine src/main.cpp src/data_feed.cpp)
target_include_directories(engine PRIVATE src)

# Symlink build/data -> project data/ so the binary finds the CSV via relative path
add_custom_command(TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/data
    COMMENT "Symlinking build/data -> project data/")