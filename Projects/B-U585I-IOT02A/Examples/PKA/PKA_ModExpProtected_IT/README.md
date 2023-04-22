## <b>PKA_ModExpProtected_IT Example Description</b>

How to use the PKA to run Protected modular exponentiation operation

This project is targeted to run on STM32U585AIIxQ devices on B-U585I-IOT02A board from STMicroelectronics.
The project configures the maximum system clock frequency at 160 MHz.

Modular exponentiation operation is commonly used to perform a single-step RSA
operation. It consists in the computation of A^e mod n.

The RNG initialization is mandatory for the PKA's operations.
To calculate (A^e mod n) with PKA we should fill the PKA_ModExpProtectModeInTypeDef with required input vectors.
The result of the operation is stored in the variable buffer.

In this example, LED7 (GREEN) is turned on when the operation finish with success
and LED6 (RED) is turned on when the operation generate an error.

#### <b>Notes</b>

 1. Care must be taken when using HAL_Delay(), this function provides accurate delay (in milliseconds)
      based on variable incremented in SysTick ISR. This implies that if HAL_Delay() is called from
      a peripheral ISR process, then the SysTick interrupt must have higher priority (numerically lower)
      than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
      To change the SysTick interrupt priority you have to use HAL_NVIC_SetPriority() function.

 2. The application needs to ensure that the SysTick time base is always set to 1 millisecond
      to have correct HAL operation.

### <b>Keywords</b>

Security, PKA, RSA, modular exponentiation, cipher, decipher

### <b>Directory contents</b>

  - PKA/PKA_ModExpProtected_IT/Src/main.c                  Main program
  - PKA/PKA_ModExpProtected_IT/Src/system_stm32u5xx.c      stm32u5xx system clock configuration file
  - PKA/PKA_ModExpProtected_IT/Src/stm32u5xx_it.c          Interrupt handlers
  - PKA/PKA_ModExpProtected_IT/Src/stm32u5xx_hal_msp.c     HAL MSP module
  - PKA/PKA_ModExpProtected_IT/Inc/main.h                  Main program header file
  - PKA/PKA_ModExpProtected_IT/Inc/b_u585i_iot02a_conf.h   BSP Configuration file
  - PKA/PKA_ModExpProtected_IT/Inc/stm32u5xx_hal_conf.h    HAL Configuration file
  - PKA/PKA_ModExpProtected_IT/Inc/stm32u5xx_it.h          Interrupt handlers header file

### <b>Hardware and Software environment</b>

  - This example runs on STM32U585AIIxQ devices without security enabled (TZEN=0).

  - This example has been tested with STMicroelectronics B-U585I-IOT02A
    board and can be easily tailored to any other supported device
    and development board.

### <b>How to use it ?</b>

In order to make the program work, you must do the following :

 - Open your preferred toolchain
 - Rebuild all files and load your image into target memory
 - Run the example

