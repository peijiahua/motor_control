#ifndef MOTOR_CAN_CONTROL_MAIN_H
#define MOTOR_CAN_CONTROL_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

#define KEY_Pin             GPIO_PIN_2
#define KEY_GPIO_Port       GPIOE
#define ETH_RESET_Pin       GPIO_PIN_3
#define ETH_RESET_GPIO_Port GPIOA
#define PWR_KEEP_Pin        GPIO_PIN_5
#define PWR_KEEP_GPIO_Port  GPIOA
#define LED_STA_Pin         GPIO_PIN_10
#define LED_STA_GPIO_Port   GPIOD
#define DIO8_Pin            GPIO_PIN_6
#define DIO8_GPIO_Port      GPIOD
#define DIO7_Pin            GPIO_PIN_7
#define DIO7_GPIO_Port      GPIOD
#define DIO6_Pin            GPIO_PIN_9
#define DIO6_GPIO_Port      GPIOG
#define DIO5_Pin            GPIO_PIN_10
#define DIO5_GPIO_Port      GPIOG
#define DIO4_Pin            GPIO_PIN_11
#define DIO4_GPIO_Port      GPIOG
#define DIO3_Pin            GPIO_PIN_12
#define DIO3_GPIO_Port      GPIOG
#define DIO2_Pin            GPIO_PIN_13
#define DIO2_GPIO_Port      GPIOG
#define DIO1_Pin            GPIO_PIN_14
#define DIO1_GPIO_Port      GPIOG
#define Read_stop_Pin       GPIO_PIN_3
#define Read_stop_GPIO_Port GPIOB

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif
