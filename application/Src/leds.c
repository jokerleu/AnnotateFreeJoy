/**
  ******************************************************************************
  * @file           : leds.c
  * @brief          : LEDs driver implementation
		
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
	
#include "leds.h"
#include "buttons.h"
	
uint8_t leds_state[MAX_LEDS_NUM];

/**
 * @brief 处理LED逻辑状态
 * 
 * @param p_dev_config 
 */
void LEDs_LogicalProcess (dev_config_t * p_dev_config)
{
	// 遍历所有LED（MAX_LEDS_NUM定义最大数量）
	for (uint8_t i=0; i<MAX_LEDS_NUM; i++)
	{
		if (p_dev_config->leds[i].input_num >= 0)
		{
			// 根据配置类型更新LED状态
			switch (p_dev_config->leds[i].type)
			{
				default:
				// 直接映射按钮状态
				case LED_NORMAL:
					leds_state[i] = logical_buttons_state[p_dev_config->leds[i].input_num].current_state;
				break;
				
				// 取反按钮状态
				case LED_INVERTED:
					leds_state[i] = !logical_buttons_state[p_dev_config->leds[i].input_num].current_state;
				break;
				
			}
		}
	}
}

/**
 * @brief 设置独立LED状态
 * 
 * @param state_buf 
 * @param p_dev_config 
 * @param pos 
 */
void LED_SetSingle(uint8_t * state_buf, dev_config_t * p_dev_config, uint8_t * pos)
{
	for (uint8_t i=0; i<USED_PINS_NUM; i++)
	{
		// 扫描所有配置为LED_SINGLE的引脚
		if (p_dev_config->pins[i] == LED_SINGLE)
		{
			// 根据leds_state数组值设置GPIO输出寄存器(ODR)，状态为1时置位引脚，状态为0时清除引脚
			leds_state[*pos] ? (pin_config[i].port->ODR |= pin_config[i].pin) : (pin_config[i].port->ODR &= ~pin_config[i].pin); 
			(*pos)++;
		}
	}
}

/**
 * @brief 控制矩阵排列LED
 * 
 * @param state_buf 
 * @param p_dev_config 
 * @param pos 
 */
void LED_SetMatrix(uint8_t * state_buf, dev_config_t * p_dev_config, uint8_t * pos)
{
	static int8_t last_row = -1; ///> 上次扫描行
	static uint8_t last_pos = 0; ///> 上次扫描位置
	int8_t max_row = -1;
	
	for (uint8_t i=0; i<USED_PINS_NUM; i++)
	{
		// turn off leds
		if (p_dev_config->pins[i] == LED_ROW)
		{
			pin_config[i].port->ODR &= ~pin_config[i].pin;
			max_row = i;
			for (uint8_t j=0; j<USED_PINS_NUM; j++)
			{
				if (p_dev_config->pins[j] == LED_COLUMN)
				{
					pin_config[j].port->ODR |= pin_config[j].pin;
					(*pos)++;
				}
			}
		}
	}
	for (uint8_t i=0; i<USED_PINS_NUM; i++)
	{
		if (p_dev_config->pins[i] == LED_ROW && (i > last_row || i == max_row))
		{
			pin_config[i].port->ODR |= pin_config[i].pin;
			for (uint8_t j=0; j<USED_PINS_NUM; j++)
			{
				if (p_dev_config->pins[j] == LED_COLUMN)
				{
					if (leds_state[last_pos++] > 0) 
					{
						pin_config[j].port->ODR &= ~pin_config[j].pin;
					}
					else
					{
						pin_config[j].port->ODR |= pin_config[j].pin; 
					}				
				}
			}
			if (last_pos >= *pos) last_pos = 0;
			(i == max_row) ? (last_row = -1): (last_row = i);
			break;
		}
		
	}
}

/**
 * @brief LED物理处理主函数
 * 
 * @param p_dev_config 
 */
void LEDs_PhysicalProcess (dev_config_t * p_dev_config)
{
	uint8_t pos = 0;
	
	// 更新逻辑状态
	LEDs_LogicalProcess(p_dev_config);
	
	// 依次处理矩阵LED(LED_SetMatrix)和独立LED(LED_SetSingle)
	LED_SetMatrix(leds_state, p_dev_config, &pos);
	LED_SetSingle(leds_state, p_dev_config, &pos);
		
}


