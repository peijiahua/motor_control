# Motor CAN Control

This is a compact STM32F407ZGTx Keil ARMCC5 project derived from the supplied `Hand_cont` project.

## Build

Open `MDK-ARM/Motor_CAN_Control.uvprojx` with Keil MDK 5 and build the `Motor_CAN_Control` target. The project uses the source project's 8 MHz HSE and 168 MHz system clock.

## Hardware mapping

| Peripheral | Pins | Timing |
| --- | --- | --- |
| CAN1 | PB8 RX, PB9 TX, AF9 | 1 Mbps, prescaler 6, BS1 4 TQ, BS2 2 TQ |
| CAN2 | PB5 RX, PB6 TX, AF9 | 1 Mbps, prescaler 6, BS1 4 TQ, BS2 2 TQ |

CAN1 is the motor bus. CAN2 is initialized with the original pin and timing configuration and is available for a second bus.

## API examples

```c
MotorControlCommand command = {50.0f, 1000U, 1.5f};
MotorControl_SetEnable(1U, 1U);
MotorControl_SetTarget(1U, &command);
MotorControl_EmergencyStop(1U);
```

Motor IDs 1 through 20 are accepted. Speed is 0 to 100 percent, where 100 percent is 3000 r/min. Position is expressed in turns and encoded as 32768 PU per turn. The transport layer applies the source project's rule that the first payload byte is sent as `0xFF` while the standard CAN ID is the motor ID.

## Source mapping

- `Core/Src/can.c` corresponds to the source `can.c` and retains CAN1/CAN2 clocks, pins, interrupts, and timing.
- `App/Src/can_port.c` consolidates the source `Easycan.c` filter, queue, callback, and transmit behavior.
- `App/Src/motor_control.c` is the focused equivalent of `motor_can_service.c`; it removes UDP parsing and UI coupling.
- `App/Src/app_freertos.c` replaces the source `freertos.c` motor and CAN tasks.
- `Core/Src/gpio.c` keeps the original board GPIO assignments.
- `LWIP/App/lwip.c` and `LWIP/Target/ethernetif.c` correspond to the source Ethernet/LwIP initialization, while `App/Src/udp_motor.c` is the focused UDP command task.

## UDP motor control

The project uses the source project's Ethernet MAC, LAN8742 RMII PHY, and LwIP setup. The default network configuration is device IP `172.23.46.5`, mask `255.255.255.0`, gateway `172.23.46.1`, and UDP port `6679`.

Send ASCII commands to `172.23.46.5:6679`:

```text
ENABLE,1,1
RUN,1,0.2,500,0.1
STOP,1
DISABLE,1
```

`RUN` fields are motor ID, speed percent, acceleration, and target position in turns. Valid commands receive `OK`; malformed, out-of-range, or CAN transport failures receive `ERR,...`. `STOP` sends the EasyCan emergency-stop command. The board no longer starts a demo motion at boot.

The board must have the LAN8742 RMII PHY connected and share the same subnet as the UDP host.
