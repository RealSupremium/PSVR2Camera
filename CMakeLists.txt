cmake_minimum_required(VERSION 3.10)
project(PSVR2Camera)

set(CMAKE_CXX_STANDARD 20)

add_executable(PSVR2Camera WIN32
    main.cpp
    distortion.cpp
    renderer.cpp
    shared_memory.cpp
)

target_link_libraries(PSVR2Camera 
    d3d11.lib 
    d3dcompiler.lib 
    dxgi.lib
)