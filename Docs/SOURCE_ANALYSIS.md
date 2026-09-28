# Source CAN and Motor Analysis

## 1. CAN initialization

The source project targets `STM32F407ZGTx` with an 8 MHz HSE and a 168 MHz system clock. APB1 is divided by four, so the CAN peripheral clock is 42 MHz. Both CAN instances use:

```text
Prescaler = 6
SJW       = 1 TQ
BS1       = 4 TQ
BS2       = 2 TQ
```

The resulting nominal rate is `42 MHz / (6 * (1 + 4 + 2)) = 1 Mbps`.

CAN1 uses PB8/PB9 with alternate function 9 and receives on FIFO0. CAN2 uses PB5/PB6 with alternate function 9 and receives on FIFO1 in the original user layer, although only CAN1 is consumed by the motor path. The new project retains the pins and timing and uses FIFO0 consistently for the focused motor transport.

The original `vApp_User_CAN1_Configuration` and `vApp_User_CAN2_Configuration` configure 32-bit standard-ID filters. Their configured ID is zero, so the mask accepts all standard IDs. The new transport uses ten 32-bit list filters on CAN1 for IDs 1 through 20, then validates the ID again in software.

## 2. IDs and frame format

The legacy layer defines motor IDs 1 through 9. The newer `motor_can_config.h` and `motor_can_service.c` define the application range as 1 through 20. The new project uses 1 through 20.

The source transmit wrapper has an unusual but deliberate convention:

1. It takes the motor ID from `aTxData[0]`.
2. It uses that value as the standard CAN ID.
3. It overwrites `aTxData[0]` with `0xFF` before the physical transmission.

The new transport copies the caller's frame, applies the same byte-zero rule without mutating the caller's buffer, and queues the standard ID separately.

## 3. Receive path

The source HAL callback calls `HAL_CAN_GetRxMessage` and puts a `CanRxQueue` object into `osQueueCanRx_CAN1`. The `motor_data` task waits on this queue and invokes both the legacy register table decoder and the newer telemetry decoder.

The newer decoder expects at least six data bytes, uses byte 0 as the motor ID and falls back to the standard CAN ID when byte 0 is not a valid ID. It accepts register `0x0F` for current and `0x11` for voltage. The 16-bit value is little-endian in bytes 4 and 5. The new project retains this behavior and stores raw value, scaled value, timestamp, valid flags, and an update sequence for every ID.

## 4. Motor initialization and control

The source does not have a separate motor-driver peripheral. Motor startup is CAN protocol startup: initialize CAN, start the CAN peripheral, enable RX notifications, create queues/tasks, and issue register requests.

The newer service layer provides these operations:

| Operation | Data bytes before transport byte-zero replacement |
| --- | --- |
| Read register | `[id, 0x03, register, 0x01]` |
| Enable/disable | `[id, 0x06, 0x01, 0x01, enable, 0x00]` |
| Speed and acceleration | `[id, 0x06, 0x02, 0x02, speed_lo, speed_hi, accel_lo, accel_hi]` |
| Absolute position | `[id, 0x06, 0x16, 0x02, pos0, pos1, pos2, pos3]` |
| Emergency stop | `[id, 0x10]` |

Speed is converted from 0 to 100 percent to 0 to 3000 r/min. Position is converted from turns to 32768 PU per turn. Multi-byte fields are little-endian.

## 5. New project mapping

`App/Src/can_port.c` owns filters, HAL callbacks, RX/TX queues, and physical transmission. `App/Src/motor_control.c` owns frame construction, input validation, telemetry parsing, and the 20-entry state cache. `App/Src/app_freertos.c` supplies the receive, transmit, polling, and UDP motor tasks. `App/Src/udp_motor.c` uses the source project's LwIP/LAN8742 setup and maps short UDP commands to the focused motor API without bringing over the DWIN UI or large legacy parameter tables.
