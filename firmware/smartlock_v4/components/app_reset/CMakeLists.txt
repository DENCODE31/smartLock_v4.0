idf_component_register(SRCS "app_reset.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver espressif__button espressif__rmaker_common)
