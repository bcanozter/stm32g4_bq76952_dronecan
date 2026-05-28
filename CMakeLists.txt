#############################################################################################################################
# file:  CMakeLists.txt
# brief: Template "CMakeLists.txt" for building of executables and static libraries.
#
# usage: Edit "VARIABLES"-section to suit project requirements.
#        For debug build:
#          cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake  -S ./ -B Debug -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
#          make -C Debug VERBOSE=1 -j
#        For release build:
#          cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake  -S ./ -B Release -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
#          make -C Release VERBOSE=1 -j
#############################################################################################################################
cmake_minimum_required(VERSION 3.20)

###################### CONSTANTS ######################################
set (PROJECT_TYPE_EXECUTABLE          "exe")
set (PROJECT_TYPE_STATIC_LIBRARY      "static-lib")
set (MCPU_CORTEX_M0				      "-mcpu=cortex-m0")
set (MCPU_CORTEX_M0PLUS				  "-mcpu=cortex-m0plus")
set (MCPU_CORTEX_M3				      "-mcpu=cortex-m3")
set (MCPU_CORTEX_M4				      "-mcpu=cortex-m4")
set (MCPU_CORTEX_M7				      "-mcpu=cortex-m7")
set (MCPU_CORTEX_M33				  "-mcpu=cortex-m33")
set (MCPU_CORTEX_M55				  "-mcpu=cortex-m55")
set (MCPU_CORTEX_M85				  "-mcpu=cortex-m85")
set (MFPU_FPV4_SP_D16                 "-mfpu=fpv4-sp-d16")
set (MFPU_FPV4                        "-mfpu=vfpv4")
set (MFPU_FPV5_D16                    "-mfpu=fpv5-d16")
set (RUNTIME_LIBRARY_REDUCED_C        "--specs=nano.specs")
set (RUNTIME_LIBRARY_STD_C            "")
set (RUNTIME_LIBRARY_SYSCALLS_MINIMAL "--specs=nosys.specs")
set (RUNTIME_LIBRARY_SYSCALLS_NONE    "")
set (MFLOAT_ABI_SOFTWARE              "-mfloat-abi=soft")
set (MFLOAT_ABI_HARDWARE              "-mfloat-abi=hard")
set (MFLOAT_ABI_MIX                   "-mfloat-abi=softfp")
#######################################################################

###################### VARIABLES ######################################
set (PROJECT_NAME             "stm32g4_bq76952_dronecan")
set (PROJECT_TYPE             "exe")
set (LINKER_SCRIPT            "../STM32G474RETX_FLASH.ld")
set (MCPU                     "-mcpu=Cortex-M4")
set (MFPU                 "-mfpu=fpv4-sp-d16")
set (MFLOAT_ABI               "")
set (RUNTIME_LIBRARY          "--specs=nano.specs")
set (RUNTIME_LIBRARY_SYSCALLS "--specs=nosys.specs")


set (PROJECT_SOURCES
	# LIST SOURCE FILES HERE
	Startup/startup_stm32g474retx.s
	Sources/main.c
	Sources/syscalls.c
	Sources/sysmem.c
	Sources/stm32g4xx_hal_msp.c
	Sources/stm32g4xx_it.c
	Sources/system_stm32g4xx.c
  Sources/canard_node.c
  Sources/canard.c
  Sources/fdcan.c
  Sources/i2c.c
  Sources/bq76952_protocol.c
	Drivers/BSP/STM32G4xx_Nucleo/stm32g4xx_nucleo.c
	Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_i2c.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_i2c_ex.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma_ex.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_exti.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_fdcan.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ramfunc.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim_ex.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_uart_ex.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_uart.c
  Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_ll_pwr.c
	)

set (PROJECT_DEFINES
	# LIST COMPILER DEFINITIONS HERE

    )

set (PROJECT_INCLUDES
	# LIST INCLUDE DIRECTORIES HERE
	Includes
  Includes/dsdl
  Includes/libcanard
  Drivers/CMSIS/Device/ST/STM32G4xx/Include
  Drivers/CMSIS/Include
  Drivers/BSP/STM32G4xx_Nucleo
  Drivers/STM32G4xx_HAL_Driver/Inc
  Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
   )
############ MODIFY ACCORDING TO REQUIREMENTS) ########################

#######################################################################

################## PROJECT SETUP ######################################
project(${PROJECT_NAME})
enable_language(ASM)

if (${PROJECT_TYPE} MATCHES ${PROJECT_TYPE_EXECUTABLE})
  add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
  file(GLOB DSDL_SOURCES
    "${CMAKE_SOURCE_DIR}/Sources/dsdl/*.c"
  )
  target_sources(${PROJECT_NAME} PRIVATE
      ${DSDL_SOURCES}
  )
  add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>)
elseif (${PROJECT_TYPE} MATCHES ${PROJECT_TYPE_STATIC_LIBRARY})
  add_library(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

add_compile_definitions (${PROJECT_DEFINES})
include_directories (${PROJECT_INCLUDES})
target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32G474xx
    USE_NUCLEO_64
)
set (CMAKE_EXECUTABLE_SUFFIX ".elf")
set (CMAKE_STATIC_LIBRARY_SUFFIX ".a")
set (CMAKE_C_FLAGS "${MCPU} -std=gnu11 ${MFPU} ${MFLOAT_ABI} ${RUNTIME_LIBRARY} -mthumb -Wall -Werror  -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter")
set (CMAKE_EXE_LINKER_FLAGS "-T${LINKER_SCRIPT} ${RUNTIME_LIBRARY_SYSCALLS} -Wl,-Map=test.map -Wl,--gc-sections -static -Wl,--start-group -lc -lm -Wl,--end-group -u _printf_float")
set (CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")
