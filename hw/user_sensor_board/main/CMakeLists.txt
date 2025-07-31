idf_component_register(
            SRCS "main.c"
            INCLUDE_DIRS "."
            REQUIRES beacon_scanner gyro_sensor heart_sensor temp_sensor
)
