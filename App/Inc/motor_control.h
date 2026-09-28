#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "can_port.h"
#include <stdint.h>

typedef enum
{
    MOTOR_STATUS_OK = 0,
    MOTOR_STATUS_INVALID_ARGUMENT,
    MOTOR_STATUS_OUT_OF_RANGE,
    MOTOR_STATUS_TRANSPORT_ERROR
} MotorStatus;

typedef struct
{
    float speed_percent;
    uint16_t acceleration;
    float position_turn;
} MotorControlCommand;

typedef struct
{
    uint8_t motor_id;
    uint8_t valid_flags;
    uint16_t current_raw;
    uint16_t voltage_raw;
    float current;
    float voltage;
    uint32_t current_update_ms;
    uint32_t voltage_update_ms;
    uint32_t update_sequence;
} MotorTelemetry;

MotorStatus MotorControl_SetTarget(uint8_t motor_id, const MotorControlCommand *command);
MotorStatus MotorControl_PreparePosition(uint8_t motor_id,
                                          uint16_t speed_rpm,
                                          uint16_t acceleration,
                                          uint8_t save_parameters);
MotorStatus MotorControl_SetEnable(uint8_t motor_id, uint8_t enable);
MotorStatus MotorControl_EmergencyStop(uint8_t motor_id);
void MotorControl_PollTelemetry(void);
void MotorControl_ProcessRx(const CanPortFrame *frame, uint32_t timestamp_ms);
MotorStatus MotorControl_GetTelemetry(uint8_t motor_id, MotorTelemetry *out);

#endif
