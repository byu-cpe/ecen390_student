idf_component_register(SRCS sound_i2s.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES esp_driver_gpio esp_driver_i2s config)
if(DEFINED EXTERN_BUF)
    target_compile_options(${COMPONENT_LIB} PRIVATE -DEXTERN_BUF=${EXTERN_BUF})
endif()
