cmake_minimum_required(VERSION 3.16)

project(littlefs LANGUAGES C CXX)

list(APPEND LITTLEFS_SRCS
	${CMAKE_SOURCE_DIR}/external/littlefs/lfs.c
	${CMAKE_SOURCE_DIR}/external/littlefs/lfs_util.c
)
list(APPEND LITTLEFS_INCS ${CMAKE_SOURCE_DIR}/external/littlefs)

add_library(${PROJECT_NAME} STATIC ${LITTLEFS_SRCS})

target_compile_features(${PROJECT_NAME} PRIVATE c_std_99)

target_include_directories(${PROJECT_NAME}
	PUBLIC ${LITTLEFS_INCS}
)
