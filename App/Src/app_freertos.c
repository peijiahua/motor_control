#include "app_freertos.h"
#include "can_port.h"
#include "motor_control.h"
#include "udp_motor_v2.h"
#include "lwip.h"

static osThreadId_t rx_task;
static osThreadId_t tx_task;
static osThreadId_t telemetry_task;
static osThreadId_t udp_motor_task;
static osThreadId_t network_init_task;

static void RxTask(void *argument)
{
    CanPortFrame frame;
    (void)argument;
    for (;;)
    {
        if (CanPort_TryReceive(&frame, osWaitForever) != 0U)
            MotorControl_ProcessRx(&frame, HAL_GetTick());
    }
}

static void TelemetryTask(void *argument)
{
    (void)argument;
    for (;;)
    {
        MotorControl_PollTelemetry();
        osDelay(10U);
    }
}

static void NetworkInitTask(void *argument)
{
    const osThreadAttr_t udp_attr = {
        .name = "udp_motor", .stack_size = 1024U, .priority = osPriorityNormal
    };
    (void)argument;
    MX_LWIP_Init();
    udp_motor_task = osThreadNew(UdpMotorV2_Task, NULL, &udp_attr);
    if (udp_motor_task == NULL) Error_Handler();
    osThreadExit();
}

void App_FreeRTOS_Init(void)
{
    const osThreadAttr_t rx_attr = {
        .name = "can_rx", .stack_size = 512U, .priority = osPriorityAboveNormal
    };
    const osThreadAttr_t tx_attr = {
        .name = "can_tx", .stack_size = 512U, .priority = osPriorityAboveNormal
    };
    const osThreadAttr_t poll_attr = {
        .name = "motor_poll", .stack_size = 512U, .priority = osPriorityNormal
    };
    const osThreadAttr_t net_attr = {
        .name = "net_init", .stack_size = 1024U, .priority = osPriorityAboveNormal
    };
    rx_task = osThreadNew(RxTask, NULL, &rx_attr);
    tx_task = osThreadNew(CanPort_TxTask, NULL, &tx_attr);
    telemetry_task = osThreadNew(TelemetryTask, NULL, &poll_attr);
    network_init_task = osThreadNew(NetworkInitTask, NULL, &net_attr);
    if ((rx_task == NULL) || (tx_task == NULL) ||
        (telemetry_task == NULL) || (network_init_task == NULL)) Error_Handler();
}
