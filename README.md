# emacs_micro_booster
Booster primera etapa 20V -> 350V

micro STM32F030K6T6

Antes de empezar revisar seleccion del micro y configuracion
------------------------------------------------------------

stm32f0_flash.cfg		//work area size y flash image

stm32f0_flash_lock.cfg		//work area size y flash image

stm32f0_gdb.cfg		        //work area size

stm32f0_reset.cfg		//work area size


./cmsis_boot/startup/stm32_flash.ld		//end of ram; stack size; memory lenght

./cmsis_boot/stm32f0xx.h					//linea 68 elegir micro


