#ifndef MOTOR_CAN_CONTROL_CAN_H
#define MOTOR_CAN_CONTROL_CAN_H

#include "main.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

void MX_CAN1_Init(void);
void MX_CAN2_Init(void);

#endif
