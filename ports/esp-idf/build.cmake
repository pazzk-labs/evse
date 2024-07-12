set(COMPONENTS_USED
	esp_psram
	freertos
	esp_eth
	esptool_py
	nvs_sec_provider
	mbedtls
	bt
	tcp_transport
	esp-tls
	esp_http_server
	esp_http_client
	esp_https_ota
	app_update
)

if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
	list(APPEND COMPONENTS_USED esp_adc)
else()
	list(APPEND COMPONENTS_USED esp_adc_cal)
endif()

idf_build_process(${IDF_TARGET}
	COMPONENTS
		${COMPONENTS_USED}
	SDKCONFIG_DEFAULTS
		"${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults"
	BUILD_DIR
		${CMAKE_CURRENT_BINARY_DIR}
)

set(mapfile "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map")
# project_description.json metadata file used for the flash and the monitor of
# idf.py to get the project information.
set(build_components_json "[]")
set(build_component_paths_json "[]")
set(common_component_reqs_json "\"\"")
set(build_component_info_json "\"\"")
set(all_component_info_json "\"\"")
configure_file("${IDF_PATH}/tools/cmake/project_description.json.in"
	"${CMAKE_CURRENT_BINARY_DIR}/project_description.json")

idf_build_set_property(COMPILE_DEFINITIONS -DtraceTASK_SWITCHED_IN=on_task_switch_in APPEND)
idf_build_set_property(COMPILE_DEFINITIONS -DxPortIsInsideInterrupt=xPortInIsrContext APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-implicit-function-declaration" APPEND)

target_link_libraries(${PROJECT_EXECUTABLE}
	idf::esp_psram
	idf::freertos
	idf::spi_flash
	idf::nvs_flash
	idf::nvs_sec_provider
	idf::driver
	idf::pthread
	idf::esp_eth
	idf::tcp_transport
	idf::mbedtls
	idf::esp_http_server
	idf::esp_http_client
	idf::esp_https_ota
	idf::app_update
	idf::esp_timer
	idf::esp_wifi

	"-Wl,--cref"
	"-Wl,--Map=\"${mapfile}\""
)
if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
target_link_libraries(${PROJECT_EXECUTABLE} idf::esp_adc)
else()
target_link_libraries(${PROJECT_EXECUTABLE} idf::esp_adc_cal)
endif()

set(idf_size ${python} $ENV{IDF_PATH}/tools/idf_size.py)
add_custom_target(size DEPENDS ${mapfile} COMMAND ${idf_size} ${mapfile})
add_custom_target(size-files DEPENDS ${mapfile} COMMAND ${idf_size} --files ${mapfile})
add_custom_target(size-components DEPENDS ${mapfile} COMMAND ${idf_size} --archives ${mapfile})

# Attach additional targets to the executable file for flashing,
# linker script generation, partition_table generation, etc.
idf_build_executable(${PROJECT_EXECUTABLE})
