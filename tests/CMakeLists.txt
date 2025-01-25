SET(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_CXX_STANDARD 17)          #Use C
set(CMAKE_CXX_STANDARD_REQUIRED ON) #Enforce the standard
set(CMAKE_CXX_EXTENSIONS OFF)       #Disable compiler-specific extensions

cmake_minimum_required(VERSION 3.8) project(mina_tests)

file(GLOB SOURCES *.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(
${PROJECT_NAME} PRIVATE ${MAKE_SOURCE_DIR}../ include)

