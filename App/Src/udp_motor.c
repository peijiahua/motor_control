#include "udp_motor.h"

#include "lwip/api.h"
#include "lwip/netbuf.h"
#include "lwip/ip_addr.h"
#include "motor_control.h"
#include "cmsis_os2.h"

#include <stdlib.h>
#include <string.h>

#define UDP_MOTOR_RX_SIZE (256U)
#define UDP_MOTOR_MOTOR_COUNT (20U)

volatile uint32_t g_udp_rx_count;
volatile uint32_t g_udp_ok_count;
volatile uint32_t g_udp_error_count;
volatile uint32_t g_udp_last_error;
static uint8_t motor_position_ready[UDP_MOTOR_MOTOR_COUNT];

static char *NextToken(char *first)
{
    return strtok(first, ",");
}

static uint8_t ParseUint(const char *text, uint32_t max_value, uint32_t *value)
{
    char *end;
    unsigned long parsed;
    if ((text == NULL) || (value == NULL) || (*text == '\0')) return 0U;
    parsed = strtoul(text, &end, 10);
    if ((*end != '\0') || (parsed > (unsigned long)max_value)) return 0U;
    *value = (uint32_t)parsed;
    return 1U;
}

static uint8_t ParseFloat(const char *text, float *value)
{
    char *end;
    double parsed;
    if ((text == NULL) || (value == NULL) || (*text == '\0')) return 0U;
    parsed = strtod(text, &end);
    if ((*end != '\0') || (parsed != parsed)) return 0U;
    *value = (float)parsed;
    return 1U;
}

static const char *ProcessCommand(char *line)
{
    char *command = NextToken(line);
    char *arg1;
    char *arg2;
    char *arg3;
    char *arg4;
    uint32_t id;
    uint32_t acceleration;
    uint32_t enable;
    float speed;
    float position;
    MotorControlCommand target;

    if (command == NULL) return "ERR,格式";
    if (strcmp(command, "RUN") == 0)
    {
        arg1 = NextToken(NULL); arg2 = NextToken(NULL);
        arg3 = NextToken(NULL); arg4 = NextToken(NULL);
        if ((arg1 == NULL) || (arg2 == NULL) || (arg3 == NULL) || (arg4 == NULL) || (NextToken(NULL) != NULL))
            return "ERR,格式";
        if (!ParseUint(arg1, 20U, &id) || (id == 0U)) return "ERR,电机ID";
        if (!ParseFloat(arg2, &speed) || !ParseUint(arg3, 60000U, &acceleration) || !ParseFloat(arg4, &position))
            return "ERR,参数";
        if ((speed < 0.0f) || (speed > 100.0f)) return "ERR,参数";
        target.speed_percent = speed;
        target.acceleration = (uint16_t)acceleration;
        target.position_turn = position;
        if (motor_position_ready[id - 1U] == 0U)
        {
            if (MotorControl_PreparePosition((uint8_t)id,
                                              (uint16_t)(speed * 30.0f),
                                              (uint16_t)acceleration,
                                              0U) != MOTOR_STATUS_OK)
                return "ERR,CAN";
            motor_position_ready[id - 1U] = 1U;
        }
        return (MotorControl_SetTarget((uint8_t)id, &target) == MOTOR_STATUS_OK) ? "OK" : "ERR,CAN";
    }

    arg1 = NextToken(NULL);
    if ((arg1 == NULL) || !ParseUint(arg1, 20U, &id) || (id == 0U)) return "ERR,电机ID";
    if (strcmp(command, "STOP") == 0)
    {
        if (NextToken(NULL) != NULL) return "ERR,格式";
        return (MotorControl_EmergencyStop((uint8_t)id) == MOTOR_STATUS_OK) ? "OK" : "ERR,CAN";
    }
    if (strcmp(command, "DISABLE") == 0)
    {
        if (NextToken(NULL) != NULL) return "ERR,格式";
        return (MotorControl_SetEnable((uint8_t)id, 0U) == MOTOR_STATUS_OK) ? "OK" : "ERR,CAN";
    }
    if (strcmp(command, "ENABLE") == 0)
    {
        arg2 = NextToken(NULL);
        if ((arg2 == NULL) || (NextToken(NULL) != NULL) || !ParseUint(arg2, 1U, &enable)) return "ERR,格式";
        return (MotorControl_SetEnable((uint8_t)id, (uint8_t)enable) == MOTOR_STATUS_OK) ? "OK" : "ERR,CAN";
    }
    return "ERR,命令";
}

static void SendReply(struct netconn *conn, const ip_addr_t *addr, uint16_t port, const char *reply)
{
    struct netbuf *out;
    size_t length = strlen(reply);
    out = netbuf_new();
    if (out == NULL) return;
    if ((netbuf_alloc(out, (u16_t)length) != NULL) && (out->p != NULL))
    {
        memcpy(out->p->payload, reply, length);
        (void)netconn_sendto(conn, out, addr, port);
    }
    netbuf_delete(out);
}

void UdpMotor_Task(void *argument)
{
    struct netconn *conn;
    struct netbuf *in;
    ip_addr_t peer_addr;
    uint16_t peer_port;
    char buffer[UDP_MOTOR_RX_SIZE];
    u16_t length;
    const char *reply;
    (void)argument;

    conn = netconn_new(NETCONN_UDP);
    if (conn == NULL)
    {
        g_udp_last_error = 1U;
        for (;;) osDelay(1000U);
    }
    if (netconn_bind(conn, IP_ADDR_ANY, MOTOR_UDP_LOCAL_PORT) != ERR_OK)
    {
        g_udp_last_error = 2U;
        netconn_delete(conn);
        for (;;) osDelay(1000U);
    }
    netconn_set_recvtimeout(conn, 100U);

    for (;;)
    {
        if (netconn_recv(conn, &in) != ERR_OK) continue;
        g_udp_rx_count++;
        memset(buffer, 0, sizeof(buffer));
        length = netbuf_copy(in, buffer, (u16_t)(sizeof(buffer) - 1U));
        if ((netbuf_len(in) >= sizeof(buffer)) || (length != netbuf_len(in)))
        {
            reply = "ERR,长度";
        }
        else
        {
            buffer[length] = '\0';
            while ((length > 0U) && ((buffer[length - 1U] == '\r') || (buffer[length - 1U] == '\n') || (buffer[length - 1U] == ' ')))
                buffer[--length] = '\0';
            reply = ProcessCommand(buffer);
        }
        if (reply[0] == 'O') g_udp_ok_count++; else g_udp_error_count++;
        peer_addr = *netbuf_fromaddr(in);
        peer_port = netbuf_fromport(in);
        SendReply(conn, &peer_addr, peer_port, reply);
        netbuf_delete(in);
    }
}
