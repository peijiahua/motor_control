#include "main.h"
#include "can.h"
#include "gpio.h"
#include "cmsis_os2.h"
#include "can_port.h"
#include "app_freertos.h"
#include "lwip.h"

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
    osc.HSEState = RCC_HSE_ON;
    osc.LSEState = RCC_LSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 4;
    osc.PLL.PLLN = 168;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    /* Keep the board power rail enabled, matching the source project's
       Key_startup power-on path before bringing up Ethernet. */
    HAL_GPIO_WritePin(PWR_KEEP_GPIO_Port, PWR_KEEP_Pin, GPIO_PIN_SET);
    MX_CAN1_Init();
    MX_CAN2_Init();
    osKernelInitialize();
    if (CanPort_Init() != HAL_OK) Error_Handler();
    App_FreeRTOS_Init();
    osKernelStart();
    for (;;) { }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED_STA_GPIO_Port, LED_STA_Pin);
        HAL_Delay(100U);
    }
}
