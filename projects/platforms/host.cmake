set(ports ports/host)
foreach(dir ${ports})
	file(GLOB_RECURSE ${dir}_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.c)
	file(GLOB_RECURSE ${dir}_CPP_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.cpp)
	list(APPEND APP_SRCS ${${dir}_SRCS} ${${dir}_CPP_SRCS})
endforeach()

if(APPLE)
list(REMOVE_ITEM APP_SRCS ports/host/apptmr.c)
else()
list(REMOVE_ITEM APP_SRCS ports/host/apptmr_ios.c)
endif()

set(PROJECT_EXECUTABLE ${CMAKE_PROJECT_NAME}.elf)
set(PROJECT_BIN ${CMAKE_PROJECT_NAME}.bin)

add_executable(${PROJECT_EXECUTABLE}
	${APP_SRCS}
)

target_compile_definitions(${PROJECT_EXECUTABLE}
	PRIVATE
		${APP_DEFS}
)

if(APPLE)
target_compile_definitions(${PROJECT_EXECUTABLE}
	PRIVATE
	    _DARWIN_C_SOURCE
)
endif()

target_include_directories(${PROJECT_EXECUTABLE}
	PRIVATE
		${APP_INCS}
		${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_EXECUTABLE}
	libmcu
	hlw811x
	ocpp
	littlefs
	cbor
	cjson
)
