/**
  ******************************************************************************
  * @file           : config.c
  * @brief          : Config management implementation 配置管理实现，负责设备配置和应用配置的存储、读取和初始化
		
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

#include "config.h"

app_config_t app_config; ///> 应用配置结构体，存储运行时配置状态

/**
 * @brief 设备配置数据有效性检查
 * 
 * @param p_dev_config 指向设备配置结构体的指针
 * @return uint8_t 0表示有效，1表示无效（空指针）
 */
uint8_t DevConfigCheck (dev_config_t * p_dev_config)
{
	uint8_t ret = 0;
	
	// 检查传入指针是否为NULL
	if (p_dev_config == NULL)	ret = 1;
	
	return ret;
}

/**
 * @brief 将设备配置写入Flash存储器
 * 
 * @param p_dev_config 指向设备配置结构体的指针
 */
void DevConfigSet (dev_config_t * p_dev_config)
{
	uint32_t data_addr = (uint32_t) p_dev_config;
	uint32_t prog_addr;
	
	// 进行有效性验证
	if (DevConfigCheck(p_dev_config) != 0)
		return;

	prog_addr = CONFIG_ADDR;
	
	// 解锁Flash编程
	FLASH_Unlock();
	// 擦除配置存储页（地址由CONFIG_ADDR定义）
	FLASH_ErasePage(prog_addr);
	
	// 按4字节对齐方式写入整个设备配置结构体
	for (int i=0; i<sizeof(dev_config_t); i+=4)
	{
		FLASH_ProgramWord(prog_addr+i, *(uint32_t *)(data_addr + i)); 

	}
	// 重新锁定Flash
	FLASH_Lock();
}

/**
 * @brief 从Flash存储器读取设备配置
 * 
 * @param p_dev_config 指向接收配置数据的目标结构体指针
 */
void DevConfigGet (dev_config_t * p_dev_config)
{
	uint32_t read_addr;
	uint32_t data_addr = (uint32_t) p_dev_config;
	
	// 检查目标指针有效性
	if (p_dev_config == NULL)
		return;
	
	//read_addr = FLASH_BASE + (*(uint16_t *)FLASHSIZE_BASE - 1)*1024;		// last page
	read_addr = CONFIG_ADDR; // 从CONFIG_ADDR地址开始读取
	// 按4字节对齐方式读取整个设备配置结构体
	for (int i=0; i<sizeof(dev_config_t); i+=4)
	{
		*(uint32_t *)(data_addr + i) = *(uint32_t *) (read_addr+i);
	}
	
}

/**
 * @brief 根据设备配置初始化应用配置
 * 
 * @param p_dev_config 指向设备配置结构体的指针
 */
void AppConfigInit (dev_config_t * p_dev_config)
{
	int8_t prev_a = -1;
	int8_t prev_b = -1;
	
	app_config.axis = 0;
	app_config.axis_cnt = 0;
	app_config.buttons_cnt = 0;
	app_config.pov = 0;
	app_config.pov_cnt = 0;
	
	// 遍历所有轴（MAX_AXIS_NUM），统计启用的轴数量和位掩码
	for (uint8_t i=0; i<MAX_AXIS_NUM; i++)
	{
		// 轴启用状态由axis_config[i].out_enabled字段决定
		if (p_dev_config->axis_config[i].out_enabled)	
		{
			app_config.axis |= (1<<i);
			app_config.axis_cnt++;
		}
	}

	for (uint8_t i=0; i<MAX_BUTTONS_NUM; i++)
	{
	
		if (p_dev_config->buttons[i].type == POV1_DOWN ||
					p_dev_config->buttons[i].type == POV1_UP ||
					p_dev_config->buttons[i].type == POV1_LEFT ||
					p_dev_config->buttons[i].type == POV1_RIGHT)
		{
			p_dev_config->buttons[i].is_disabled = 1;
			app_config.pov |= 0x01;
		}
		else if (p_dev_config->buttons[i].type == POV2_DOWN ||
					p_dev_config->buttons[i].type == POV2_UP ||
					p_dev_config->buttons[i].type == POV2_LEFT ||
					p_dev_config->buttons[i].type == POV2_RIGHT)
		{
			p_dev_config->buttons[i].is_disabled = 1;
			app_config.pov |= 0x02;
		}
		else if (p_dev_config->buttons[i].type == POV3_DOWN ||
					p_dev_config->buttons[i].type == POV3_UP ||
					p_dev_config->buttons[i].type == POV3_LEFT ||
					p_dev_config->buttons[i].type == POV3_RIGHT)
		{
			p_dev_config->buttons[i].is_disabled = 1;
			app_config.pov |= 0x04;
		}
		else if (p_dev_config->buttons[i].type == POV4_DOWN ||
					p_dev_config->buttons[i].type == POV4_UP ||
					p_dev_config->buttons[i].type == POV4_LEFT ||
					p_dev_config->buttons[i].type == POV4_RIGHT)
		{
			p_dev_config->buttons[i].is_disabled = 1;
			app_config.pov |= 0x08;
		}
		
		app_config.pov_cnt = ((app_config.pov & 0x08)>>3) + ((app_config.pov & 0x04)>>2) + 
												 ((app_config.pov & 0x02)>>1) + (app_config.pov & 0x01);
		
		if (!p_dev_config->buttons[i].is_disabled && p_dev_config->buttons[i].physical_num >=0)
		{
			app_config.buttons_cnt++;
		}
	}
	
	if (p_dev_config->pins[8] == FAST_ENCODER &&
			p_dev_config->pins[9] == FAST_ENCODER)
	{
		app_config.fast_encoder_cnt++;
	}
	
	for (int i=0; i<MAX_BUTTONS_NUM; i++)
	{
		if ((p_dev_config->buttons[i].type) == ENCODER_INPUT_A &&  i > prev_a)
		{
			for (int j=0; j<MAX_BUTTONS_NUM; j++)
			{
				if ((p_dev_config->buttons[j].type) == ENCODER_INPUT_B && j > prev_b && app_config.slow_encoder_cnt < MAX_ENCODERS_NUM - 1)
				{
					app_config.slow_encoder_cnt++;
					break;
				}
			}
		}	
	}
	
	for (uint8_t i=0; i<USED_PINS_NUM; i++)
	{
		if (p_dev_config->pins[i] == LED_PWM)	app_config.pwm_cnt++;
	}
}

/**
 * @brief 获取当前应用配置的副本
 * 
 * @param p_app_config 
 */
void AppConfigGet (app_config_t * p_app_config)
{
	memcpy(p_app_config, &app_config, sizeof(app_config_t));
}

/**
 * @brief 检查应用配置是否为空（无效）
 * 
 * @param p_app_config 
 * @return uint8_t 
 */
uint8_t IsAppConfigEmpty (app_config_t * p_app_config)
{
	// 轴数量、按钮数量和POV数量都为0时返回1（空），否则返回0
	if (p_app_config->axis_cnt == 0 && p_app_config->buttons_cnt == 0 &&
		  p_app_config->pov_cnt == 0) return 1;
	return 0;
}

