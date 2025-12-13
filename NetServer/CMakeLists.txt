cmake_minimum_required(VERSION 3.10)

project(SimpleMMO_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# libsodium 경로 설정 (프로젝트 내 ThirdParty 폴더)
set(LIBSODIUM_ROOT "${CMAKE_SOURCE_DIR}/../Plugins/GsNetworking/Source/ThirdParty/libsodium")
set(LIBSODIUM_INCLUDE_DIR "${LIBSODIUM_ROOT}/include")
set(LIBSODIUM_LIB_DIR "${LIBSODIUM_ROOT}/lib/windows-x64")

# 소스 파일 추가
set(SOURCES
    main.cpp
    Protocol.h
    Crypto.h
    Session.h
)

# 실행 파일 생성
add_executable(SimpleMMO_Server ${SOURCES})

# Include 경로 추가
target_include_directories(SimpleMMO_Server PRIVATE ${LIBSODIUM_INCLUDE_DIR})

# 라이브러리 링크
if(WIN32)
    target_link_libraries(SimpleMMO_Server 
        ws2_32
        "${LIBSODIUM_LIB_DIR}/libsodium.lib"
    )
endif()

# 복사: DLL이 필요할 경우 (static lib는 필요 없음)
# 현재는 static library이므로 추가 복사 불필요
