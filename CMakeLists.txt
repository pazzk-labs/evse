cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
	cmake_policy(SET CMP0135 NEW)
endif()

set(TARGET_PLATFORM "" CACHE STRING "Target platform (esp32s3, host)")
if (NOT TARGET_PLATFORM)
	if (DEFINED ENV{TARGET_PLATFORM})
		set(TARGET_PLATFORM $ENV{TARGET_PLATFORM})
	else()
		message(FATAL_ERROR "TARGET_PLATFORM is not set. Use -DTARGET_PLATFORM=<platform> or set the TARGET_PLATFORM environment variable.")
	endif()
endif()

set(PLATFORM_PRE_CMAKE_FILE "${CMAKE_SOURCE_DIR}/projects/platforms/${TARGET_PLATFORM}-pre.cmake")
if (EXISTS ${PLATFORM_PRE_CMAKE_FILE})
	include(${PLATFORM_PRE_CMAKE_FILE})
endif()

project(EVSE)

include(${CMAKE_SOURCE_DIR}/projects/version.cmake)
include(${CMAKE_SOURCE_DIR}/projects/sources.cmake)
include(${CMAKE_SOURCE_DIR}/projects/external.cmake)

set(PLATFORM_CMAKE_FILE "${CMAKE_SOURCE_DIR}/projects/platforms/${TARGET_PLATFORM}.cmake")
if (NOT EXISTS ${PLATFORM_CMAKE_FILE})
	message(FATAL_ERROR "Platform CMake file not found: ${PLATFORM_CMAKE_FILE}")
endif()

include(${PLATFORM_CMAKE_FILE})
