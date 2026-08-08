# Package Health Monitor

A small device that rides inside a shipped parcel and tells you exactly how it was handled in transit, very important for certain parcels for example Home lab tests where vibrations in the blood sample leads to hemolysis and interferes with the lab reports. Built on an STM32 Nucleo-C5A3ZG with an MPU6050 IMU, it turns raw motion and temperature data into a clean, live health score — no cloud, no app, just a UART cable and real numbers.

## What it does

- Samples the MPU6050 at 200 Hz for accurate, real-time motion tracking
- Detects hard shocks and separates them cleanly from ongoing vibration, even when both happen in the same motion
- Tracks orientation via a complementary filter, flagging packages left tilted too long
- Monitors internal temperature against a safe operating range
- Converts every event into a running 0–100 health score
- Starts and stops per-shipment with a single button press
- Streams clean, readable telemetry and instant event alerts over UART — no debugger required

## Hardware

| Part | Role |
|---|---|
| NUCLEO-C5A3ZG | STM32C5A3ZG, Cortex-M33, up to 144 MHz |
| MPU6050 | Accelerometer + gyroscope + on-die temperature, over I2C |
| Onboard button | Trip start/stop |
| ST-LINK V3EC | Programming and UART debug console over a single USB cable |

**Wiring:** I2C1 on PB6 (SCL) / PB7 (SDA). Debug UART on USART2 / PA2, 115200 8N1, routed straight through the onboard ST-LINK virtual COM port.

## How it works

Sampling runs on a 1 kHz SysTick hook that paces the IMU reads at 200 Hz. Every sample flows through the health monitor, which computes acceleration magnitude, tilt angle, and vibration RMS, and raises clean, edge-triggered events — a shock is only a shock once, a tilt only counts once it's held for two full seconds, vibration only counts once it's sustained across multiple windows. Those events feed straight into the scoring engine, which deducts points immediately and keeps a running total for the trip.

**Shock vs. vibration:** the trickiest part of the whole system. A single fast motion produces both a sharp acceleration spike and a burst of high-frequency energy, so naively they'd double-trigger. This is solved with two rules: the vibration window blanks for 400 ms right after any shock, and vibration only counts once it's held above threshold for three consecutive 200 ms windows. The result is a system that reliably tells "one hard knock" apart from "an hour on a bad road."

## Scoring

| Event | Deduction |
|---|---|
| Moderate shock (≥2.5g) | −5 |
| Severe shock (≥4g) | −15 |
| Vibration event / sustained | −1, then −1 every 10s |
| Improper tilt / sustained | −10, then −10 every 5s |
| Temperature excursion / sustained | −10, then −10 every 60s |

## Example output

```
[124500 ms] ACC(g): x=0.02 y=0.01 z=0.98 | GYRO(dps): x=1.1 y=-0.3 z=0.2 | TEMP=24.3C | TILT=3.2deg | SCORE=100 | STATUS=OK
!!SHOCK!! level=2 peak=4.70g t=126100 ms SCORE=85
TRIP STARTED / TRIP ENDED
```

## Build & flash

Standard STM32CubeIDE project for the NUCLEO-C5A3ZG. Open, build, and flash over the onboard ST-LINK via USB — the same cable gives you the live 115200 baud debug console.

## Why it's built this way

Each module has exactly one job: the IMU driver only talks I2C, the health monitor only turns raw samples into events, the scoring engine only turns events into points, and the logger only formats what it's handed. That separation is what keeps the shock/vibration logic reliable — every layer stays simple enough to reason about on its own, which is what makes the whole pipeline hold together end to end.

##VIDEO LINK
https://uploadnow.io/f/YRPqJwG
