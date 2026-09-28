#ifndef MOTOR_CAN_PORT_H
#define MOTOR_CAN_PORT_H

#include "can.h"
#include <stdint.h>

typedef struct
{
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];
} CanPortFrame;

extern volatile uint32_t g_can_tx_count;
extern volatile uint32_t g_can_tx_error_count;
extern volatile uint32_t g_can_rx_count;
extern volatile uint32_t g_can_rx_dropped_count;
extern volatile uint32_t g_can_last_error;

HAL_StatusTypeDef CanPort_Init(void);
HAL_StatusTypeDef CanPort_QueueTx(uint32_t standard_id, const uint8_t *data, uint8_t length);
uint8_t CanPort_TryReceive(CanPortFrame *frame, uint32_t timeout);
void CanPort_TxTask(void *argument);

#endif
