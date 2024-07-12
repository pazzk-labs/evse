add_subdirectory(external/libmcu)
add_subdirectory(external/hlw811x)
add_subdirectory(external/ocpp)
add_subdirectory(external/cbor)
add_subdirectory(ports/littlefs)
add_subdirectory(ports/cjson)

if(APPLE)
	set(LIBMCU_NOINIT __attribute__\(\(section\(\"__DATA,.noinit.libmcu\"\)\)\))
else()
	set(LIBMCU_NOINIT __attribute__\(\(section\(\".noinit.libmcu\"\)\)\))
endif()

target_compile_definitions(libmcu
	PUBLIC
		_POSIX_THREADS
		_POSIX_C_SOURCE=200809L
		LIBMCU_NOINIT=${LIBMCU_NOINIT}
		METRICS_USER_DEFINES=\"${PROJECT_SOURCE_DIR}/include/metrics.def\"
		WDT_STACK_SIZE_BYTES=4096
		LOGGING_MAX_BACKENDS=2

		${APP_DEFS}

	PRIVATE
		WDT_ERROR=error
		RETRY_INFO=info
)
target_compile_options(libmcu
	PRIVATE
		-include ${PROJECT_SOURCE_DIR}/external/libmcu/modules/logging/include/libmcu/logging.h
)

target_compile_definitions(littlefs PUBLIC LFS_THREADSAFE)

target_compile_definitions(ocpp
	PRIVATE
		OCPP_CONFIGURATION_DEFINES=\"ocpp_config.def\"
		OCPP_LIBRARY_VERSION=1
)
target_include_directories(ocpp PRIVATE ${PROJECT_SOURCE_DIR}/include)
