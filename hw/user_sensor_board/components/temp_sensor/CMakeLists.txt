idf_component_register(
    SRCS "src/temp_sensor_task.c"
         "src/mlx90614_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES driver
)
