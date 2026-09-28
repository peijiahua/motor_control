#ifndef MOTOR_UDP_CONTROL_H
#define MOTOR_UDP_CONTROL_H

#include <stdint.h>

#define MOTOR_UDP_LOCAL_PORT (6679U)

extern volatile uint32_t g_udp_rx_count;
extern volatile uint32_t g_udp_ok_count;
extern volatile uint32_t g_udp_error_count;
extern volatile uint32_t g_udp_last_error;

void UdpMotor_Task(void *argument);

#endif
