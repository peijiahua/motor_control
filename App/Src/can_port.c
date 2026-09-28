#include "can_port.h"
#include "motor_config.h"
#include "cmsis_os2.h"
#include <string.h>

typedef struct
{
    uint32_t standard_id;
    uint8_t length;
    uint8_t data[8];
} CanPortTxFrame;

static osMessageQueueId_t rx_queue;
static osMessageQueueId_t tx_queue;
volatile uint32_t g_can_tx_count;
volatile uint32_t g_can_tx_error_count;
volatile uint32_t g_can_rx_count;
volatile uint32_t g_can_rx_dropped_count;
volatile uint32_t g_can_last_error;

static uint16_t FilterWord(uint32_t id)
{
    return (uint16_t)((id & 0x7FFU) << 5U);
}

static HAL_StatusTypeDef ConfigureCan1Filters(void)
{
    CAN_FilterTypeDef filter;
    uint32_t bank;
    uint32_t first_id;
    uint32_t second_id;
    HAL_StatusTypeDef status = HAL_OK;

    for (bank = 0U; bank < 10U; bank++)
    {
        first_id = MOTOR_CAN_MIN_ID + (bank * 2U);
        second_id = first_id + 1U;
        memset(&filter, 0, sizeof(filter));
        filter.FilterBank = bank;
        filter.FilterMode = CAN_FILTERMODE_IDLIST;
        filter.FilterScale = CAN_FILTERSCALE_32BIT;
        filter.FilterIdHigh = FilterWord(first_id);
        filter.FilterIdLow = 0U;
        filter.FilterMaskIdHigh = FilterWord(second_id);
        filter.FilterMaskIdLow = 0U;
        filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        filter.FilterActivation = ENABLE;
        filter.SlaveStartFilterBank = 14U;
        status = HAL_CAN_ConfigFilter(&hcan1, &filter);
        if (status != HAL_OK) return status;
    }
    /* EasyCan replies use the host standard ID 0xFF. */
    memset(&filter, 0, sizeof(filter));
    filter.FilterBank = 10U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = FilterWord(0xFFU);
    filter.FilterMaskIdHigh = FilterWord(0x7FFU);
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;
    status = HAL_CAN_ConfigFilter(&hcan1, &filter);
    return status;
}

static HAL_StatusTypeDef ConfigureCan2Filter(void)
{
    CAN_FilterTypeDef filter;
    memset(&filter, 0, sizeof(filter));
    filter.FilterBank = 14U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;
    return HAL_CAN_ConfigFilter(&hcan2, &filter);
}

HAL_StatusTypeDef CanPort_Init(void)
{
    rx_queue = osMessageQueueNew(32U, sizeof(CanPortFrame), NULL);
    tx_queue = osMessageQueueNew(32U, sizeof(CanPortTxFrame), NULL);
    if ((rx_queue == NULL) || (tx_queue == NULL)) return HAL_ERROR;
    if (ConfigureCan1Filters() != HAL_OK) return HAL_ERROR;
    if (ConfigureCan2Filter() != HAL_OK) return HAL_ERROR;
    if (HAL_CAN_Start(&hcan1) != HAL_OK) return HAL_ERROR;
    if (HAL_CAN_Start(&hcan2) != HAL_OK) return HAL_ERROR;
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) return HAL_ERROR;
    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) return HAL_ERROR;
    return HAL_OK;
}

HAL_StatusTypeDef CanPort_QueueTx(uint32_t standard_id, const uint8_t *data, uint8_t length)
{
    CanPortTxFrame frame;
    if ((data == NULL) || (length > 8U) || (standard_id > 0x7FFU)) return HAL_ERROR;
    frame.standard_id = standard_id;
    frame.length = length;
    memset(frame.data, 0, sizeof(frame.data));
    memcpy(frame.data, data, length);
    if (length > 0U) frame.data[0] = 0xFFU;
    return (osMessageQueuePut(tx_queue, &frame, 0U, 0U) == osOK) ? HAL_OK : HAL_BUSY;
}

uint8_t CanPort_TryReceive(CanPortFrame *frame, uint32_t timeout)
{
    if ((frame == NULL) || (rx_queue == NULL)) return 0U;
    return (uint8_t)(osMessageQueueGet(rx_queue, frame, NULL, timeout) == osOK);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CanPortFrame frame;
    uint32_t fifo = CAN_RX_FIFO0;
    if (HAL_CAN_GetRxMessage(hcan, fifo, &frame.header, frame.data) != HAL_OK) return;
    g_can_rx_count++;
    if ((rx_queue == NULL) || (osMessageQueuePut(rx_queue, &frame, 0U, 0U) != osOK))
        g_can_rx_dropped_count++;
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan == &hcan1)
        g_can_last_error = HAL_CAN_GetError(hcan);
}

void CanPort_TxTask(void *argument)
{
    CanPortTxFrame frame;
    CAN_TxHeaderTypeDef header;
    uint32_t mailbox;
    (void)argument;
    for (;;)
    {
        if (osMessageQueueGet(tx_queue, &frame, NULL, osWaitForever) == osOK)
        {
            memset(&header, 0, sizeof(header));
            header.StdId = frame.standard_id;
            header.IDE = CAN_ID_STD;
            header.RTR = CAN_RTR_DATA;
            header.DLC = frame.length;
            header.TransmitGlobalTime = DISABLE;
            while (HAL_CAN_AddTxMessage(&hcan1, &header, frame.data, &mailbox) != HAL_OK)
            {
                g_can_tx_error_count++;
                g_can_last_error = HAL_CAN_GetError(&hcan1);
                osDelay(1U);
            }
            g_can_tx_count++;
            g_can_last_error = HAL_CAN_GetError(&hcan1);
        }
    }
}
