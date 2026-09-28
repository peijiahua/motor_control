# Validation

## Static checks

- Device is `STM32F407ZGTx`.
- CAN1 is PB8/PB9 AF9 and CAN2 is PB5/PB6 AF9.
- CAN timing is prescaler 6, SJW 1 TQ, BS1 4 TQ, BS2 2 TQ.
- The project includes Core, App, HAL, CMSIS, startup, FreeRTOS, Ethernet/LAN8742, and LwIP files.
- Motor IDs are limited to 1 through 20.
- UDP listens on `172.23.46.5:6679` and accepts `RUN`, `ENABLE`, `DISABLE`, and `STOP` commands.

## Protocol checks

- Read request: `[id, 0x03, register, 0x01]`.
- Enable request: `[id, 0x06, 0x01, 0x01, enable, 0x00]`.
- Speed/acceleration write starts at `0x02`; position write starts at `0x16`.
- Emergency stop is `[id, 0x10]`.
- Telemetry parser accepts registers `0x0F` and `0x11`, with values in little-endian bytes 4 and 5.

## UDP checks

- `RUN,1,0.2,500,0.1` prepares motor 1 on its first use and queues the target position.
- Invalid or out-of-range commands return `ERR,...` without queuing CAN data.
- `STOP,1` queues the EasyCan emergency-stop frame.

## Toolchain

The repository environment does not provide a Keil executable. Build verification therefore consists of project XML parsing, source membership checks, and review against the source HAL/protocol constants. Run the target build in Keil MDK 5 on Windows before flashing hardware.
