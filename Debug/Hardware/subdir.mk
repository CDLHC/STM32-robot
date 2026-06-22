################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Hardware/BOO.c \
../Hardware/Input.c \
../Hardware/LED.c \
../Hardware/NRF24L01.c \
../Hardware/OLED.c \
../Hardware/OLED_Data.c \
../Hardware/PWM.c \
../Hardware/Serial.c \
../Hardware/Servo.c \
../Hardware/WS2812B.c 

OBJS += \
./Hardware/BOO.o \
./Hardware/Input.o \
./Hardware/LED.o \
./Hardware/NRF24L01.o \
./Hardware/OLED.o \
./Hardware/OLED_Data.o \
./Hardware/PWM.o \
./Hardware/Serial.o \
./Hardware/Servo.o \
./Hardware/WS2812B.o 

C_DEPS += \
./Hardware/BOO.d \
./Hardware/Input.d \
./Hardware/LED.d \
./Hardware/NRF24L01.d \
./Hardware/OLED.d \
./Hardware/OLED_Data.d \
./Hardware/PWM.d \
./Hardware/Serial.d \
./Hardware/Servo.d \
./Hardware/WS2812B.d 


# Each subdirectory must supply rules for building sources it contributes
Hardware/%.o Hardware/%.su Hardware/%.cyclo: ../Hardware/%.c Hardware/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"D:/STM32Cube  project/robot/Hardware" -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Hardware

clean-Hardware:
	-$(RM) ./Hardware/BOO.cyclo ./Hardware/BOO.d ./Hardware/BOO.o ./Hardware/BOO.su ./Hardware/Input.cyclo ./Hardware/Input.d ./Hardware/Input.o ./Hardware/Input.su ./Hardware/LED.cyclo ./Hardware/LED.d ./Hardware/LED.o ./Hardware/LED.su ./Hardware/NRF24L01.cyclo ./Hardware/NRF24L01.d ./Hardware/NRF24L01.o ./Hardware/NRF24L01.su ./Hardware/OLED.cyclo ./Hardware/OLED.d ./Hardware/OLED.o ./Hardware/OLED.su ./Hardware/OLED_Data.cyclo ./Hardware/OLED_Data.d ./Hardware/OLED_Data.o ./Hardware/OLED_Data.su ./Hardware/PWM.cyclo ./Hardware/PWM.d ./Hardware/PWM.o ./Hardware/PWM.su ./Hardware/Serial.cyclo ./Hardware/Serial.d ./Hardware/Serial.o ./Hardware/Serial.su ./Hardware/Servo.cyclo ./Hardware/Servo.d ./Hardware/Servo.o ./Hardware/Servo.su ./Hardware/WS2812B.cyclo ./Hardware/WS2812B.d ./Hardware/WS2812B.o ./Hardware/WS2812B.su

.PHONY: clean-Hardware

