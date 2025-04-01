if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	set(COMMON_COMPILE_OPTIONS
		-Werror
		-Wall
		-Wextra
		-Wpedantic
		-Wc++-compat
		-Wformat=2
		-Wmissing-prototypes
		-Wstrict-prototypes
		-Wmissing-declarations
		-Wcast-align
		-Wpointer-arith
		-Wbad-function-cast
		-Wcast-qual
		-Wmissing-format-attribute
		-Wmissing-include-dirs
		-Wformat-nonliteral
		-Wdouble-promotion
		-Wfloat-equal
		-Winline
		-Wundef
		-Wunused-macros
		-Wshadow
		-Wwrite-strings
		-Waggregate-return
		-Wredundant-decls
		-Wconversion
		-Wstrict-overflow=5
		-Wno-long-long
		-Wswitch-default
	)
	# Additional code quality and optimization related options
	list(APPEND COMMON_COMPILE_OPTIONS
		-Wnull-dereference
		-Wsign-conversion
		-Wstack-protector
		-Walloca
		#-Wpadded
		-Wdisabled-optimization
		-Wimplicit-fallthrough
	)
	# Handle errors as warnings
	list(APPEND COMMON_COMPILE_OPTIONS
	)
	if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		list(APPEND COMMON_COMPILE_OPTIONS
			-Wno-error=deprecated-declarations
		)
	endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	list(APPEND COMMON_COMPILE_OPTIONS
		-Wlogical-op
		-Wduplicated-cond
		-Wduplicated-branches
		-Wrestrict
		-Wunsafe-loop-optimizations
		-Wvector-operation-performance
		-Wsuggest-attribute=pure
		-Wsuggest-attribute=const
		-Wsuggest-attribute=noreturn
		-Wsuggest-attribute=format
		-Wstack-usage=1024
		-Wuseless-cast
		-Wsuggest-final-types
		-Wsuggest-final-methods
	)
endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	# MSVC specific warning options (different from GCC/Clang)
	set(COMMON_COMPILE_OPTIONS
		/W4          # highest warning level
		/WX          # turn warnings into errors
		/permissive- # apply strict standard conformance
		/sdl         # stronger security checks
		/analyze     # static code analysis
		/wd4127      # turn off warning C4127: conditional expression is constant
		/we4013      # Treat warning for using undeclared functions as an error
		/w44263      # Detect missing cases in `switch`
		/w44265      # Warning for hidden virtual functions
		/w4456       # Warning for local variable shadowing
		/w4457       # Warning for function parameter shadowing
		/w4477       # Detect `printf` format errors
		/w4505       # Detect unused static functions
		/w4668       # Detect usage of undefined macros
		/w4710       # Warning for functions not inlined
		/w4711       # Warning for automatically inlined functions
		/w4820       # Detect structure padding
		/w5026       # Warning if move constructor is not implicitly defined
		/w5039       # Warning related to C++ exceptions
	)
endif()
