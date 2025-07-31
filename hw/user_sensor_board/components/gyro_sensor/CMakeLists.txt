idf_component_register(
    SRCS "src/gyro_sensor_task.c"
         "src/mpu6050_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES driver
)
