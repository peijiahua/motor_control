#include "motor_control.h"
#include "motor_config.h"
#include "cmsis_os2.h"
#include <stddef.h>

static MotorTelemetry telemetry[MOTOR_CAN_COUNT];
static volatile uint32_t telemetry_sequence[MOTOR_CAN_COUNT];
static uint8_t poll_index;
static uint8_t poll_voltage;

static uint8_t IsValidId(uint8_t id)
{
    return (uint8_t)((id >= MOTOR_CAN_MIN_ID) && (id <= MOTOR_CAN_MAX_ID));
}

static MotorStatus QueueFrame(uint8_t id, const uint8_t *data, uint8_t length)
{
    return (CanPort_QueueTx(id, data, length) == HAL_OK) ? MOTOR_STATUS_OK : MOTOR_STATUS_TRANSPORT_ERROR;
}

static MotorStatus WriteRegister16(uint8_t id, uint8_t reg, uint16_t value)
{
    uint8_t frame[6] = {id, MOTOR_CAN_FUNC_WRITE, reg, 1U, 0U, 0U};
    frame[4] = (uint8_t)value;
    frame[5] = (uint8_t)(value >> 8U);
    return QueueFrame(id, frame, 6U);
}

MotorStatus MotorControl_PreparePosition(uint8_t id,
                                          uint16_t speed_rpm,
                                          uint16_t acceleration,
                                          uint8_t save_parameters)
{
    MotorStatus status;
    if (!IsValidId(id)) return MOTOR_STATUS_INVALID_ARGUMENT;

    /* EasyCan position-mode startup sequence from the reference implementation. */
    status = WriteRegister16(id, MOTOR_CAN_REG_MODBUS_ENABLE, 1U);
    if (status != MOTOR_STATUS_OK) return status;
    osDelay(20U);
    status = WriteRegister16(id, MOTOR_CAN_REG_MODE, 0U);
    if (status != MOTOR_STATUS_OK) return status;
    osDelay(20U);
    status = WriteRegister16(id, MOTOR_CAN_REG_TARGET_SPEED, speed_rpm);
    if (status != MOTOR_STATUS_OK) return status;
    osDelay(20U);
    status = WriteRegister16(id, MOTOR_CAN_REG_ACCELERATION, acceleration);
    if (status != MOTOR_STATUS_OK) return status;
    osDelay(20U);
    if (save_parameters != 0U)
    {
        status = WriteRegister16(id, MOTOR_CAN_REG_SAVE_PARAMETERS, 1U);
        if (status != MOTOR_STATUS_OK) return status;
        osDelay(20U);
    }
    return MotorControl_SetEnable(id, 1U);
}

MotorStatus MotorControl_SetTarget(uint8_t id, const MotorControlCommand *command)
{
    uint8_t frame[8];
    uint16_t speed;
    int32_t position;
    if ((!IsValidId(id)) || (command == NULL)) return MOTOR_STATUS_INVALID_ARGUMENT;
    if ((command->speed_percent != command->speed_percent) ||
        (command->position_turn != command->position_turn) ||
        (command->speed_percent < 0.0f) || (command->speed_percent > 100.0f) ||
        (command->acceleration > MOTOR_CAN_MAX_ACCELERATION)) return MOTOR_STATUS_OUT_OF_RANGE;
    if ((command->position_turn > 65535.0f) || (command->position_turn < -65536.0f)) return MOTOR_STATUS_OUT_OF_RANGE;
    speed = (uint16_t)(command->speed_percent * (MOTOR_CAN_SPEED_RPM_AT_100_PCT / 100.0f));
    position = (int32_t)(command->position_turn * MOTOR_CAN_POSITION_PU_PER_TURN);
    frame[0] = id; frame[1] = MOTOR_CAN_FUNC_WRITE; frame[2] = MOTOR_CAN_REG_TARGET_SPEED; frame[3] = 1U;
    frame[4] = (uint8_t)speed; frame[5] = (uint8_t)(speed >> 8U);
    if (QueueFrame(id, frame, 6U) != MOTOR_STATUS_OK) return MOTOR_STATUS_TRANSPORT_ERROR;
    frame[2] = MOTOR_CAN_REG_ACCELERATION;
    frame[4] = (uint8_t)command->acceleration; frame[5] = (uint8_t)(command->acceleration >> 8U);
    if (QueueFrame(id, frame, 6U) != MOTOR_STATUS_OK) return MOTOR_STATUS_TRANSPORT_ERROR;
    frame[2] = MOTOR_CAN_REG_ABSOLUTE_POSITION;
    frame[3] = 2U;
    frame[4] = (uint8_t)position; frame[5] = (uint8_t)((uint32_t)position >> 8U);
    frame[6] = (uint8_t)((uint32_t)position >> 16U); frame[7] = (uint8_t)((uint32_t)position >> 24U);
    return QueueFrame(id, frame, 8U);
}

MotorStatus MotorControl_SetEnable(uint8_t id, uint8_t enable)
{
    uint8_t frame[6] = {id, MOTOR_CAN_FUNC_WRITE, MOTOR_CAN_REG_ENABLE, 1U, enable, 0U};
    if ((!IsValidId(id)) || (enable > 1U)) return MOTOR_STATUS_INVALID_ARGUMENT;
    return QueueFrame(id, frame, 6U);
}

MotorStatus MotorControl_EmergencyStop(uint8_t id)
{
    uint8_t frame[2] = {id, MOTOR_CAN_FUNC_EMERGENCY_STOP};
    if (!IsValidId(id)) return MOTOR_STATUS_INVALID_ARGUMENT;
    return QueueFrame(id, frame, 2U);
}

void MotorControl_PollTelemetry(void)
{
    uint8_t frame[4];
    uint8_t id = (uint8_t)(MOTOR_CAN_MIN_ID + poll_index);
    frame[0] = id; frame[1] = MOTOR_CAN_FUNC_READ;
    frame[2] = (poll_voltage != 0U) ? MOTOR_CAN_REG_SYSTEM_VOLTAGE : MOTOR_CAN_REG_SYSTEM_CURRENT;
    frame[3] = 1U;
    (void)QueueFrame(id, frame, 4U);
    poll_voltage ^= 1U;
    if (poll_voltage == 0U)
    {
        poll_index++;
        if (poll_index >= MOTOR_CAN_COUNT) poll_index = 0U;
    }
}

void MotorControl_ProcessRx(const CanPortFrame *frame, uint32_t timestamp_ms)
{
    uint8_t id;
    uint8_t reg;
    uint16_t raw;
    MotorTelemetry *item;
    if ((frame == NULL) || (frame->header.IDE != CAN_ID_STD) || (frame->header.DLC < 6U)) return;
    id = frame->data[0];
    if (!IsValidId(id)) id = (uint8_t)frame->header.StdId;
    if (!IsValidId(id)) return;
    reg = frame->data[2];
    if ((reg != MOTOR_CAN_REG_SYSTEM_CURRENT) && (reg != MOTOR_CAN_REG_SYSTEM_VOLTAGE)) return;
    raw = (uint16_t)frame->data[4] | ((uint16_t)frame->data[5] << 8U);
    item = &telemetry[id - MOTOR_CAN_MIN_ID];
    telemetry_sequence[id - MOTOR_CAN_MIN_ID]++;
    item->motor_id = id;
    if (reg == MOTOR_CAN_REG_SYSTEM_CURRENT)
    {
        item->current_raw = raw; item->current = (float)raw; item->current_update_ms = timestamp_ms;
        item->valid_flags |= MOTOR_CAN_TELEMETRY_CURRENT;
    }
    else
    {
        item->voltage_raw = raw; item->voltage = (float)raw; item->voltage_update_ms = timestamp_ms;
        item->valid_flags |= MOTOR_CAN_TELEMETRY_VOLTAGE;
    }
    telemetry_sequence[id - MOTOR_CAN_MIN_ID]++;
    item->update_sequence = telemetry_sequence[id - MOTOR_CAN_MIN_ID];
}

MotorStatus MotorControl_GetTelemetry(uint8_t id, MotorTelemetry *out)
{
    uint32_t before;
    uint32_t after;
    if ((!IsValidId(id)) || (out == NULL)) return MOTOR_STATUS_INVALID_ARGUMENT;
    do
    {
        before = telemetry_sequence[id - MOTOR_CAN_MIN_ID];
        *out = telemetry[id - MOTOR_CAN_MIN_ID];
        after = telemetry_sequence[id - MOTOR_CAN_MIN_ID];
    } while (((before & 1U) != 0U) || (before != after));
    out->motor_id = id;
    return MOTOR_STATUS_OK;
}
