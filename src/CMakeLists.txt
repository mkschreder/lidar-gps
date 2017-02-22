cmake_minimum_required (VERSION 2.8.11)

project (LGPS NONE)

enable_language(C ASM)

set(CMAKE_SYSTEM Generic)

file(GLOB stm32-sources 
	stm32f10x/*.c
	stm32f10x/src/*.c
	stm32f10x/usb_vcp/*.c
	stm32f10x/usb_otg/src/*.c
	stm32f10x/usb_core/Core/src/*.c
	stm32f10x/usb_core/Class/cdc/src/*.c
)
file(GLOB littlebot-sources
	littlebot/*.c
)
file(GLOB hiltester-sources
	hiltester/*.c
)
file(GLOB freertos-sources FreeRTOS/Source/*.c FreeRTOS/Source/portable/GCC/ARM_CM3/*.c FreeRTOS/Source/portable/MemMang/heap_2.c)
file(GLOB main-sources apps/*.c libbot/*.c *.c)

set(INCLUDE_DIRS . CMSIS/  littlebot stm32f10x/)
set(FREERTOS_DIRS FreeRTOS/Source/include FreeRTOS/Source/portable/GCC/ARM_CM3/)
set(STM32_CFLAGS 
	-specs=nosys.specs 
	-mcpu=cortex-m3
	-mthumb
	-mlittle-endian
	-mfloat-abi=soft
	-mthumb-interwork
	${STARTUP_CFLAGS}
)
set(STM32_LDFLAGS -Wl,--gc-sections -Wl,-L${CMAKE_CURRENT_SOURCE_DIR}/stm32f10x/ -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT} )
set(COMMON_FLAGS -std=gnu99 -Werror -g -O0 -Wall -Wextra -ffunction-sections -fdata-sections -include "config.h")

set(COVERAGE_CFLAGS "-fprofile-arcs -ftest-coverage")
set(COVERAGE_LDFLAGS "-lgcov")

set(LGPS_ELF lgps.elf)
set(LGPS_HEX lgps.hex)
set(LGPS_BIN lgps.bin)
 
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

add_executable(${LGPS_ELF} ${main-sources} ${stm32-sources} ${freertos-sources} ${littlebot-sources} ${STARTUP_SOURCES})
target_include_directories(${LGPS_ELF} PRIVATE ${INCLUDE_DIRS} ${FREERTOS_DIRS})
target_compile_options(${LGPS_ELF} PRIVATE ${COMMON_FLAGS} ${STM32_CFLAGS})
target_link_libraries(${LGPS_ELF} LINK_PUBLIC m ${STM32_CFLAGS} ${STM32_LDFLAGS})
add_custom_command(TARGET ${LGPS_ELF} POST_BUILD
	COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${LGPS_ELF}> ${LGPS_HEX}
	COMMENT "Firmware is in ${LGPS_HEX}"
)
add_custom_command(TARGET ${LGPS_ELF} POST_BUILD
	COMMAND arm-none-eabi-objcopy -j .text -j .data -O binary $<TARGET_FILE:${LGPS_ELF}> ${LGPS_BIN}
	COMMENT "Binary firmware is in ${LGPS_BIN}"
)

