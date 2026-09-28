#include "gpio.h"

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, ETH_RESET_Pin | PWR_KEEP_Pin | GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_STA_GPIO_Port, LED_STA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);

    gpio.Pin = KEY_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(KEY_GPIO_Port, &gpio);

    gpio.Pin = ETH_RESET_Pin | PWR_KEEP_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = LED_STA_Pin;
    HAL_GPIO_Init(LED_STA_GPIO_Port, &gpio);

    gpio.Pin = DIO8_Pin | DIO7_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOD, &gpio);
    gpio.Pin = DIO6_Pin | DIO5_Pin | DIO4_Pin | DIO3_Pin | DIO2_Pin | DIO1_Pin;
    HAL_GPIO_Init(GPIOG, &gpio);
    gpio.Pin = Read_stop_Pin;
    HAL_GPIO_Init(Read_stop_GPIO_Port, &gpio);

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio);
    gpio.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOA, &gpio);
}
