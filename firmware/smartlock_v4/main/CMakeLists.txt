idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")
target_compile_definitions(${COMPONENT_LIB} PRIVATE HELTEC_V3)
