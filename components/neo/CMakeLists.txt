idf_component_register(SRCS neo.c led_strip_encoder.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES driver config)
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
