set(src-dirs
	src
	ports/libmcu
	ports/lis2dw12
	ports/tca9539
	ports/tmp102
)

foreach(dir ${src-dirs})
	file(GLOB_RECURSE ${dir}_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.c)
	file(GLOB_RECURSE ${dir}_CPP_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.cpp)
	list(APPEND SRCS_TMP ${${dir}_SRCS} ${${dir}_CPP_SRCS})
endforeach()

set(APP_SRCS
	${SRCS_TMP}
)
set(APP_INCS
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/include/driver
)
set(APP_DEFS
	BUILD_DATE=${BUILD_DATE}
	VERSION_TAG_MAJOR=${VERSION_MAJOR}
	VERSION_TAG_MINOR=${VERSION_MINOR}
	VERSION_TAG_PATCH=${VERSION_PATCH}
	VERSION_TAG=${VERSION_TAG}

	_POSIX_THREADS
	_POSIX_C_SOURCE=200809L
	_GNU_SOURCE # for strptime and settimeofday

	LOGGING_MESSAGE_MAXLEN=1500
)
