cmake_minimum_required(VERSION 3.20)
project(Avventura1 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/src/launcher.cpp)
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-static)
add_executable(avventura1 WIN32
  src/main.cpp
  src/tap.cpp
  src/vm.cpp
  src/screen.cpp
  src/ui.cpp
  src/launcher.cpp
  src/choices.cpp
)
target_include_directories(avventura1 PRIVATE src)
target_link_libraries(avventura1 PRIVATE SDL3::SDL3-static)
if(MINGW)
  target_link_options(avventura1 PRIVATE -static -static-libgcc -static-libstdc++)
endif()
endif()

# test senza finestra: esegue la VM e stampa lo schermo come testo
add_executable(vmtest src/vmtest.cpp src/tap.cpp src/vm.cpp src/screen.cpp src/choices.cpp)
target_include_directories(vmtest PRIVATE src)
if(MINGW)
  target_link_options(vmtest PRIVATE -static -static-libgcc -static-libstdc++)
endif()
