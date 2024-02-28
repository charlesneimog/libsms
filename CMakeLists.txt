cmake_minimum_required(VERSION 3.5)
project(sms)

add_definitions(-DHAVE_FFTW3_H)
set(libs m fftw3 gsl gslcblas)
add_definitions(-DMEXP=19937)

# add_library(sms SHARED ${SOURCES})

file(GLOB SOURCES "src/*.c")
add_library(sms ${SOURCES})
target_link_libraries(sms ${libs})

