cmake_minimum_required(VERSION 3.16)

project(cjson LANGUAGES C CXX)

list(APPEND CJSON_SRCS
	${CMAKE_SOURCE_DIR}/external/cJSON/cJSON.c
	${CMAKE_SOURCE_DIR}/external/cJSON/cJSON_Utils.c
)
list(APPEND CJSON_INCS ${CMAKE_SOURCE_DIR}/external/cJSON)

add_library(${PROJECT_NAME} STATIC ${CJSON_SRCS})

target_compile_features(${PROJECT_NAME} PRIVATE c_std_99)

target_include_directories(${PROJECT_NAME}
	PUBLIC ${CJSON_INCS}
)
