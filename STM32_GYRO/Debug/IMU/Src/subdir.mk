################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../IMU/Src/imu.c 

OBJS += \
./IMU/Src/imu.o 

C_DEPS += \
./IMU/Src/imu.d 


# Each subdirectory must supply rules for building sources it contributes
IMU/Src/%.o IMU/Src/%.su IMU/Src/%.cyclo: ../IMU/Src/%.c IMU/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../IMU/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-IMU-2f-Src

clean-IMU-2f-Src:
	-$(RM) ./IMU/Src/imu.cyclo ./IMU/Src/imu.d ./IMU/Src/imu.o ./IMU/Src/imu.su

.PHONY: clean-IMU-2f-Src

