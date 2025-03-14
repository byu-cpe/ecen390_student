if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/rx.c)
idf_component_register(SRCS rx.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES esp_adc main)
# After build, copy the archive file to current directory
add_custom_command(TARGET ${COMPONENT_LIB}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${COMPONENT_LIB}> ${CMAKE_CURRENT_LIST_DIR}
                   COMMENT "Copying built archive file to rx directory...")
else()
idf_component_register(INCLUDE_DIRS .)
add_prebuilt_library(rx "librx.a" PRIV_REQUIRES esp_adc main)
target_link_libraries(${COMPONENT_LIB} INTERFACE rx)
endif()
