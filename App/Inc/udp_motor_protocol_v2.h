#ifndef UDP_MOTOR_PROTOCOL_V2_H
#define UDP_MOTOR_PROTOCOL_V2_H

#include "stdint.h"

#define UDP_V2_MOTOR_COUNT       9U
#define UDP_V2_MOTOR_FRAME_SIZE  9U
#define UDP_V2_SYSTEM_FRAME_SIZE 9U
#define UDP_V2_STATE_ROWS        31U
#define UDP_V2_STATE_COLS       9U
#define UDP_V2_STATE_COUNT      (UDP_V2_STATE_ROWS * UDP_V2_STATE_COLS)

typedef struct
{
    uint8_t id;
    uint8_t valid;
    float speed_percent;
    uint16_t acceleration;
    float position_turn;
} UdpV2MotorCommand;

typedef struct
{
    uint8_t id;
    uint8_t origin;
    uint8_t read_position;
    int8_t enable;
    int8_t stop;
    uint8_t read_speed;
    uint8_t read_current;
} UdpV2SystemCommand;

int UdpV2_ParseMotorFrame(const uint8_t *frame, uint16_t length, UdpV2MotorCommand *out);
int UdpV2_ParseSystemFrame(const uint8_t *frame, uint16_t length, UdpV2SystemCommand *out);
uint16_t UdpV2_PackStateCsv(const float state[UDP_V2_STATE_COUNT], char *buffer, uint16_t capacity);

#endif
