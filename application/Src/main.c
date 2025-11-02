
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
	
		FreeJoy software for game device controllers
    Copyright (C) 2020  Yury Vostrenkov (yuvostrenkov@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
		
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "periphery.h"
#include "config.h"
#include "analog.h"
#include "buttons.h"
#include "leds.h"
#include "encoders.h"

#include "usb_hw.h"
#include "usb_lib.h"
#include "usb_pwr.h"


/* Private variables ---------------------------------------------------------*/
dev_config_t dev_config; ///< 这是行尾注释
/** @brief 简单注释测试 */
volatile uint8_t bootloader = 0;

/* Private function prototypes -----------------------------------------------*/

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{	
	// Relocate vector table
	WRITE_REG(SCB->VTOR, 0x8002000);
	
	SysTick_Init();
	
	// getting configuration from flash memory
	DevConfigGet(&dev_config);
	
	// set default config at first startup
	if ((dev_config.firmware_version & 0xFFF0) != (FIRMWARE_VERSION &0xFFF0))
	{
		DevConfigSet((dev_config_t *) &init_config);
		DevConfigGet(&dev_config);
	}
	AppConfigInit(&dev_config);
	
	USB_HW_Init();
	// wait for USB initialization
	Delay_ms(1000);	
	
	IO_Init(&dev_config);
	 
	EncodersInit(&dev_config);	
	ShiftRegistersInit(&dev_config);
	RadioButtons_Init(&dev_config);
	SequentialButtons_Init(&dev_config);		
	
	// init sensors
	AxesInit(&dev_config);
	// start sequential periphery reading
	Timers_Init(&dev_config);		
	
  while (1)
  {		
		ButtonsDebounceProcess(&dev_config);
		ButtonsReadLogical(&dev_config);
		
		// LED物理处理主函数
		LEDs_PhysicalProcess(&dev_config);
		
		analog_data_t tmp[8];
		AnalogGet(NULL, tmp, NULL);
		PWM_SetFromAxis(&dev_config, tmp);
		
		// Enter flasher command received
		if (bootloader > 0)
		{
			// Disable HID report generation 禁用指定外部中断的中断线路
			NVIC_DisableIRQ(TIM2_IRQn);
			Delay_ms(50);	// time to let HID end last transmission 让HID结束最后一次传输
			// Disable USB
			PowerOff();
			USB_HW_DeInit();
			Delay_ms(500);	
			EnterBootloader();
		}
  }
}

/**
  * @brief  Jumping to memory address corresponding bootloader program 跳转到相应引导加载程序的内存地址
  * @param  None
  * @retval None
  */
void EnterBootloader (void)
{
	/* Enable the power and backup interface clocks by setting the
	 * PWREN and BKPEN bits in the RCC_APB1ENR register 通过在RCC_APB1ENR寄存器中设置PWREN和BKPEN位来启用电源和备份接口时钟
	 */
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);

	/* Enable write access to the backup registers and the
		* RTC. 启用对备份寄存器和RTC的写访问
		*/
	SET_BIT(PWR->CR, PWR_CR_DBP);
	WRITE_REG(BKP->DR4, 0x424C);
	CLEAR_BIT(PWR->CR, PWR_CR_DBP);
	
	CLEAR_BIT(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);
	
	// 发起系统重置请求
	NVIC_SystemReset();
}


/**
  * @}
  */

/**
  * @}
  */

