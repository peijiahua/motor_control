#include "udp_motor_v2.h"
#include "udp_motor_protocol_v2.h"
#include "motor_control.h"
#include "lwip/api.h"
#include "lwip/netbuf.h"
#include "cmsis_os2.h"
#include <string.h>

#define UDP_V2_PORT 6679U
#define UDP_V2_RX_MAX 128U
volatile uint32_t g_udp_v2_rx_count, g_udp_v2_ok_count, g_udp_v2_error_count;

static void reply(struct netconn *c, const ip_addr_t *a, u16_t p, const char *s)
{
    struct netbuf *b = netbuf_new(); u16_t n = (u16_t)strlen(s);
    if (b != NULL) { if (netbuf_alloc(b,n)!=NULL) { memcpy(b->p->payload,s,n); (void)netconn_sendto(c,b,a,p); } netbuf_delete(b); }
}

void UdpMotorV2_Task(void *argument)
{
    struct netconn *c; struct netbuf *in; ip_addr_t a; u16_t port, n; uint8_t raw[UDP_V2_RX_MAX];
    UdpV2MotorCommand mc; UdpV2SystemCommand sc; MotorControlCommand target;
    (void)argument; c=netconn_new(NETCONN_UDP); if(c==NULL) for(;;) osDelay(1000U);
    if(netconn_bind(c,IP_ADDR_ANY,UDP_V2_PORT)!=ERR_OK) { netconn_delete(c); for(;;) osDelay(1000U); }
    netconn_set_recvtimeout(c,100U);
    for(;;) {
        if(netconn_recv(c,&in)!=ERR_OK) continue; g_udp_v2_rx_count++;
        n=netbuf_copy(in,raw,sizeof(raw)); a=*netbuf_fromaddr(in); port=netbuf_fromport(in);
        if(n==UDP_V2_MOTOR_FRAME_SIZE && UdpV2_ParseMotorFrame(raw,n,&mc)==0) {
            target.speed_percent=mc.speed_percent; target.acceleration=mc.acceleration; target.position_turn=mc.position_turn;
            if(MotorControl_SetTarget(mc.id,&target)==MOTOR_STATUS_OK) { reply(c,&a,port,"OK"); g_udp_v2_ok_count++; }
            else { reply(c,&a,port,"ERR,CAN"); g_udp_v2_error_count++; }
        } else if(n==UDP_V2_SYSTEM_FRAME_SIZE && UdpV2_ParseSystemFrame(raw,n,&sc)==0) {
            MotorStatus st=MOTOR_STATUS_OK;
            if(sc.stop==1) st=MotorControl_EmergencyStop(sc.id); else if(sc.enable>=0) st=MotorControl_SetEnable(sc.id,(uint8_t)sc.enable);
            if(st==MOTOR_STATUS_OK) { reply(c,&a,port,"OK"); g_udp_v2_ok_count++; } else { reply(c,&a,port,"ERR,CAN"); g_udp_v2_error_count++; }
        } else { reply(c,&a,port,"ERR,FRAME"); g_udp_v2_error_count++; }
        netbuf_delete(in);
    }
}
