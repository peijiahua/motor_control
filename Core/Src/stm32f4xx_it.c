#include "main.h"
#include "can.h"
#include "lwip.h"

void CAN1_RX0_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }
void CAN1_RX1_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }
void CAN2_RX0_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan2); }
void ETH_IRQHandler(void) { HAL_ETH_IRQHandler(&heth); }
