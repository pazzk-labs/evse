set(ports ports/esp-idf ports/freertos)
foreach(dir ${ports})
	file(GLOB_RECURSE ${dir}_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.c)
	file(GLOB_RECURSE ${dir}_CPP_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.cpp)
	list(APPEND PORT_SRCS ${${dir}_SRCS} ${${dir}_CPP_SRCS})
endforeach()

set(PROJECT_EXECUTABLE ${CMAKE_PROJECT_NAME}.elf)
set(PROJECT_BIN ${CMAKE_PROJECT_NAME}.bin)

include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

set(LIBMCU_ROOT ${PROJECT_SOURCE_DIR}/external/libmcu)
list(APPEND PORT_SRCS
	${LIBMCU_ROOT}/ports/esp-idf/board.c
	${LIBMCU_ROOT}/ports/esp-idf/actor.c
	${LIBMCU_ROOT}/ports/esp-idf/pthread.c
	${LIBMCU_ROOT}/ports/esp-idf/wifi.c
	${LIBMCU_ROOT}/ports/esp-idf/metrics.c
	${LIBMCU_ROOT}/ports/esp-idf/nvs_kvstore.c
	${LIBMCU_ROOT}/ports/esp-idf/cpuload.c
	${LIBMCU_ROOT}/ports/esp-idf/uart.c
	${LIBMCU_ROOT}/ports/esp-idf/i2c.c
	${LIBMCU_ROOT}/ports/esp-idf/pwm.c
	${LIBMCU_ROOT}/ports/esp-idf/spi.c
	${LIBMCU_ROOT}/ports/esp-idf/adc.c
	${LIBMCU_ROOT}/ports/esp-idf/apptmr.c
	${LIBMCU_ROOT}/ports/esp-idf/wdt.c
	${LIBMCU_ROOT}/ports/freertos/timext.c
	${LIBMCU_ROOT}/ports/posix/logging.c
	${LIBMCU_ROOT}/ports/posix/button.c
	${LIBMCU_ROOT}/ports/metrics/cbor_encoder.c
	${LIBMCU_ROOT}/ports/mbedtls/pki.c
)

if ($ENV{IDF_VERSION} VERSION_LESS "5.1.0")
	list(APPEND PORT_SRCS ${LIBMCU_ROOT}/ports/freertos/semaphore.c)
endif()

add_executable(${PROJECT_EXECUTABLE} ${APP_SRCS} ${PORT_SRCS})

target_compile_definitions(${PROJECT_EXECUTABLE}
	PRIVATE
		${APP_DEFS}
		LWIP
		WDT_ERROR=error
		ESP_PLATFORM
	PUBLIC
		${IDF_TARGET}
)

target_include_directories(${PROJECT_EXECUTABLE}
	PRIVATE
		${APP_INCS}
		${CMAKE_CURRENT_LIST_DIR}

		$ENV{IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos
		$ENV{IDF_PATH}/components/freertos/include/freertos

		${CMAKE_SOURCE_DIR}/ports/esp-idf/websocket/include

		${LIBMCU_ROOT}/ports/mbedtls/include
)

target_link_libraries(${PROJECT_EXECUTABLE}
	libmcu
	hlw811x
	ocpp
	littlefs
	cbor
	cjson
)

target_compile_options(${PROJECT_EXECUTABLE}
	PRIVATE
		-include ${LIBMCU_ROOT}/modules/logging/include/libmcu/logging.h
)

set(ADDITIONAL_COMPILE_OPTIONS
	"-Wno-error=redundant-decls"
	"-Wno-error=missing-include-dirs"
	"-Wno-error"
)
set_source_files_properties(${APP_SRCS}
	PROPERTIES COMPILE_OPTIONS
		"${COMMON_COMPILE_OPTIONS};${ADDITIONAL_COMPILE_OPTIONS}"
)

target_include_directories(libmcu PUBLIC
	${LIBMCU_ROOT}/modules/common/include/libmcu/posix)

include(${CMAKE_SOURCE_DIR}/ports/esp-idf/build.cmake)
