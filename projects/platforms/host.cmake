list(REMOVE_ITEM COMMON_COMPILE_OPTIONS
	-Wuseless-cast
	-Wsuggest-final-types
	-Wsuggest-final-methods
)
list(APPEND COMMON_COMPILE_OPTIONS -Wno-error=stack-protector)

list(REMOVE_ITEM APP_SRCS
	src/app.c
	src/pilot.c
	src/pinmap.c
	src/periph.c
	src/relay.c
	src/usrinp.c
	src/driver/adc122s051.c
	src/metering/adapter/hlw8112.c
)

set(ports ports/host)
foreach(dir ${ports})
	file(GLOB_RECURSE ${dir}_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.c)
	file(GLOB_RECURSE ${dir}_CPP_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.cpp)
	list(APPEND APP_SRCS ${${dir}_SRCS} ${${dir}_CPP_SRCS})
endforeach()

if(APPLE)
list(REMOVE_ITEM APP_SRCS ports/host/apptmr.c)
list(APPEND APP_SRCS ${CMAKE_SOURCE_DIR}/external/libmcu/ports/apple/semaphore.c)
else()
list(REMOVE_ITEM APP_SRCS ports/host/apptmr_ios.c)
endif()

set(PROJECT_EXECUTABLE ${CMAKE_PROJECT_NAME}.elf)
set(PROJECT_BIN ${CMAKE_PROJECT_NAME}.bin)

add_executable(${PROJECT_EXECUTABLE}
	${APP_SRCS}
	${CMAKE_SOURCE_DIR}/external/libmcu/examples/memory_kvstore.c
	${CMAKE_SOURCE_DIR}/external/libmcu/ports/posix/actor.c
	${CMAKE_SOURCE_DIR}/external/libmcu/ports/openssl/pki.c
)

target_compile_definitions(${PROJECT_EXECUTABLE} PRIVATE ${APP_DEFS})
if(APPLE)
target_compile_definitions(${PROJECT_EXECUTABLE} PRIVATE _DARWIN_C_SOURCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENSSL REQUIRED openssl)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	pkg_check_modules(LWS REQUIRED libwebsockets)
	target_include_directories(${PROJECT_EXECUTABLE} PRIVATE ${LWS_INCLUDE_DIRS})
	target_link_directories(${PROJECT_EXECUTABLE} PRIVATE ${LWS_LIBRARY_DIRS})
	target_link_libraries(${PROJECT_EXECUTABLE} PRIVATE ${LWS_LIBRARIES})
else()
	set(LWS_WITH_CLIENT ON CACHE BOOL "Enable client support")
	set(LWS_WITH_SSL ON CACHE BOOL "Enable SSL support")
	set(LWS_WITHOUT_TESTAPPS ON CACHE BOOL "Disable test apps")
	include(FetchContent)
	FetchContent_Declare(
		libwebsockets
		GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
		GIT_TAG        v4.4.1
	)
	FetchContent_MakeAvailable(libwebsockets)
	target_link_libraries(${PROJECT_EXECUTABLE} PRIVATE websockets_shared)
endif()


target_include_directories(${PROJECT_EXECUTABLE}
	PRIVATE
		${APP_INCS}
		${CMAKE_CURRENT_LIST_DIR}
		${CMAKE_SOURCE_DIR}/external/libmcu/modules/buzzer/include
		${CMAKE_SOURCE_DIR}/external/libmcu/examples

		${OPENSSL_INCLUDE_DIRS}
)

target_link_directories(${PROJECT_EXECUTABLE}
	PRIVATE
		${OPENSSL_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_EXECUTABLE}
	PRIVATE
		warnings

		libmcu
		hlw811x
		ocpp
		littlefs
		cbor
		cjson

		${OPENSSL_LIBRARIES}
)

find_program(OBJCOPY_EXECUTABLE NAMES llvm-objcopy gobjcopy objcopy)

if(NOT OBJCOPY_EXECUTABLE)
	message(FATAL_ERROR "objcopy not found. Please install binutils or LLVM tools.")
endif()

add_custom_command(
	OUTPUT ${PROJECT_BIN}
	COMMAND ${OBJCOPY_EXECUTABLE} -O binary ${PROJECT_EXECUTABLE} ${PROJECT_BIN}
	DEPENDS ${PROJECT_EXECUTABLE}
	COMMENT "Generating ${PROJECT_BIN} from ${PROJECT_EXECUTABLE}"
)

add_custom_target(bin ALL DEPENDS ${PROJECT_BIN})
