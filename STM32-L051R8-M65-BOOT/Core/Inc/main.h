/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define NTC_ACTIVE_Pin GPIO_PIN_0
#define NTC_ACTIVE_GPIO_Port GPIOC
#define RGB_LED_BLUE_PWM_Pin GPIO_PIN_4
#define RGB_LED_BLUE_PWM_GPIO_Port GPIOA
#define RGB_LED_GREEN_PWM_Pin GPIO_PIN_5
#define RGB_LED_GREEN_PWM_GPIO_Port GPIOA
#define RGB_LED_RED_PWM_Pin GPIO_PIN_6
#define RGB_LED_RED_PWM_GPIO_Port GPIOA
#define PWRKEY_CONTROL_Pin GPIO_PIN_12
#define PWRKEY_CONTROL_GPIO_Port GPIOB
#define DC_DC_POWER_ON_OFF_Pin GPIO_PIN_9
#define DC_DC_POWER_ON_OFF_GPIO_Port GPIOC
#define DISTANCE_SENSOR_ON_OFF_Pin GPIO_PIN_8
#define DISTANCE_SENSOR_ON_OFF_GPIO_Port GPIOA
#define SIM_DETECT_POWER_Pin GPIO_PIN_6
#define SIM_DETECT_POWER_GPIO_Port GPIOB
#define SIM_DETECT_Pin GPIO_PIN_7
#define SIM_DETECT_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
