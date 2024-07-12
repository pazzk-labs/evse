if (NOT DEFINED IDF_TARGET)
	set(IDF_TARGET esp32s3)
endif()
set(CMAKE_TOOLCHAIN_FILE $ENV{IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)
