cmake_minimum_required(VERSION 3.10)

project(SimpleMMO_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 소스 파일 추가
set(SOURCES
    main.cpp
    Protocol.h
)

# 실행 파일 생성
add_executable(SimpleMMO_Server ${SOURCES})

# Windows Socket 라이브러리 링크
if(WIN32)
    target_link_libraries(SimpleMMO_Server ws2_32)
endif()
