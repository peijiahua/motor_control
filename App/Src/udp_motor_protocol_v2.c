#include "udp_motor_protocol_v2.h"
#include "stdio.h"

static int valid_id(uint8_t id) { return id >= 1U && id <= UDP_V2_MOTOR_COUNT; }

int UdpV2_ParseMotorFrame(const uint8_t *frame, uint16_t length, UdpV2MotorCommand *out)
{
    if (frame == 0 || out == 0 || length != UDP_V2_MOTOR_FRAME_SIZE || !valid_id(frame[0])) return -1;
    if (frame[1] > 1U || frame[2] > 100U) return -2;
    out->id = frame[0]; out->valid = frame[1]; out->speed_percent = (float)frame[2];
    out->acceleration = (uint16_t)frame[3] | ((uint16_t)frame[4] << 8);
    out->position_turn = (float)((int16_t)((uint16_t)frame[5] | ((uint16_t)frame[6] << 8))) / 100.0f;
    return 0;
}

int UdpV2_ParseSystemFrame(const uint8_t *frame, uint16_t length, UdpV2SystemCommand *out)
{
    if (frame == 0 || out == 0 || length != UDP_V2_SYSTEM_FRAME_SIZE || !valid_id(frame[0])) return -1;
    if (frame[1] > 1U || frame[2] > 1U || frame[5] > 1U || frame[6] > 1U) return -2;
    if (frame[3] > 1U && frame[3] != 0xFFU) return -2;
    if (frame[4] > 1U && frame[4] != 0xFFU) return -2;
    out->id = frame[0]; out->origin = frame[1]; out->read_position = frame[2];
    out->enable = (frame[3] == 0xFFU) ? -1 : (int8_t)frame[3];
    out->stop = (frame[4] == 0xFFU) ? -1 : (int8_t)frame[4];
    out->read_speed = frame[5]; out->read_current = frame[6]; return 0;
}

uint16_t UdpV2_PackStateCsv(const float state[UDP_V2_STATE_COUNT], char *buffer, uint16_t capacity)
{
    uint16_t used = 0U; uint16_t i; int n;
    if (state == 0 || buffer == 0 || capacity == 0U) return 0U;
    for (i = 0U; i < UDP_V2_STATE_COUNT; ++i) {
        n = snprintf(&buffer[used], (size_t)(capacity - used), (i == 0U) ? "%.4f" : ",%.4f", (double)state[i]);
        if (n < 0 || (uint16_t)n >= (uint16_t)(capacity - used)) { buffer[0] = '\0'; return 0U; }
        used = (uint16_t)(used + (uint16_t)n);
    }
    return used;
}
