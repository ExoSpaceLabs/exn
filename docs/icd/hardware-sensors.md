# EXN Reference Hardware Sensor and Health ICD

This document defines the **normative Service 3 hardware-sensor and platform-health catalogue** for the reference EXN hardware.

It complements:

- `../component-services.md` — common HK/SENSORS wire model;
- `mcu-rtos.md` — OBC ownership;
- `linux-payload.md` — Raspberry Pi / PI-MGMT ownership;
- `fpga-ai.md` — Kria/FPGA ownership.

The rule is deliberately concrete: if EXN selects a reference board and that board provides a useful hardware monitor, the ICD names it, assigns a stable sensor ID, defines its unit and says which endpoint owns it.

## 1. Sensor versus health metric

**SENSORS** contains physical measurements:

- junction/SoC temperature;
- supply voltage;
- supply current;
- supply power;
- battery voltage where physically measurable.

**HEALTH** contains computed/software/platform state:

- CPU load;
- RAM/storage availability;
- throttling or undervoltage flags;
- reset reason;
- watchdog state;
- service state;
- FPGA image/configuration state;
- queue/error counters.

A value does not become a physical sensor merely because Linux exposes it in `/sys`.

## 2. Hardware profile IDs

| ID | Profile | Owner endpoint |
|---:|---|---|
| `0x0001` | STM32H755 / NUCLEO-H755ZI-Q reference OBC | OBC `0x100` |
| `0x0101` | Raspberry Pi 5 reference payload computer | PI-MGMT `0x104` |
| `0x0201` | AMD Kria K26 SOM | FPGA `0x102` |
| `0x0202` | AMD Kria KV260 Starter Kit | FPGA `0x102` |

The KV260 profile includes the K26 SOM profile plus carrier-specific telemetry.

Future boards get new profile IDs and sensor tables here before their values are used in mission telemetry.

## 3. Common SENSORS payload

After the common `TM[3,25]` prefix for structure `SENSORS (3)`, the payload is:

```text
hardware_profile_id:u16
count:u16
repeat count:
    sensor_id:u16
    status:u8
    alarm_state:u8
    value_type:u8
    value_length:u16
    value[value_length]
```

Sensor IDs are stable within the endpoint hardware family.

Normative engineering units are defined by the sensor ID and therefore are not repeated on every sample:

- temperature: signed **centi-degrees Celsius (cC)**;
- voltage: unsigned **millivolts (mV)**;
- current: signed **milliamps (mA)** unless a profile declares directionless current;
- power: unsigned **milliwatts (mW)**.

Implementations convert native driver units to these units before packet encoding.

A required reference-profile sensor returning `UNAVAILABLE` is a platform/driver degradation to be reported in HEALTH. It is not silently replaced with zero.

## 4. STM32H755 OBC profile — 0x0001

The STM32H755 contains an internal temperature sensor connected to ADC3_IN18 and factory calibration data. It also exposes the internal voltage reference and VBAT measurement path.

| Sensor ID | Name | Type/unit | Requirement | Source |
|---:|---|---|---|---|
| `0x0101` | `obc.mcu_junction_temp` | I32 cC | required | internal temperature sensor + factory calibration |
| `0x0102` | `obc.vdda_estimate` | U32 mV | required once ADC HK is enabled | VREFINT-derived VDDA estimate |
| `0x0103` | `obc.vbat` | U32 mV | optional | ADC VBAT internal channel when VBAT is actually connected/meaningful |

The OBC HEALTH extension should additionally report reset/watchdog cause and both Cortex-M core/runtime state. These are health fields, not sensors.

## 5. Raspberry Pi 5 profile — 0x0101

PI-MGMT owns Pi board telemetry. Camera and Antenna shall not duplicate host-wide Pi sensors.

### 5.1 Required physical sensors

| Sensor ID | Name | Type/unit | Requirement | Reference source |
|---:|---|---|---|---|
| `0x1101` | `pi5.soc_temp` | I32 cC | required | Linux thermal zone / firmware temperature |
| `0x1102` | `pi5.ext5v` | U32 mV | required | PMIC ADC `EXT5V_V` |
| `0x1103` | `pi5.core_voltage` | U32 mV | optional capability | PMIC ADC core-voltage channel when exposed by the installed PMIC/firmware |

Driver/interface selection for SoC temperature and PMIC measurements belongs in [the Pi implementation architecture](https://github.com/ExoSpaceLabs/exn-pi-cam/blob/main/docs/PI_NODE_ARCHITECTURE.md). This ICD defines the IDs, units and required reporting, independent of acquisition mechanism.

### 5.2 PI-MGMT HEALTH extension

The following are useful platform state and belong in HEALTH rather than SENSORS:

```text
cpu_load_permille:u16
arm_clock_hz:u32
memory_available_bytes:u64
payload_store_free_bytes:u64
update_store_free_bytes:u64
throttle_flags:u32
reset_reason:u8
usb_overcurrent_seen:u8
camera_service_state:u8
antenna_service_state:u8
```

`throttle_flags` is an EXN-normalized bitfield:

| Bit | Meaning |
|---:|---|
| 0 | input undervoltage active |
| 1 | CPU frequency currently capped |
| 2 | thermal throttling active |
| 3 | soft temperature limit active |
| 16 | input undervoltage occurred since boot |
| 17 | CPU frequency capping occurred since boot |
| 18 | thermal throttling occurred since boot |
| 19 | soft temperature limit occurred since boot |

The Pi implementation maps native firmware/kernel status into the **EXN bitfield above**; acquisition commands and raw platform encodings are owned in the Pi repository.

`reset_reason` values:

| Value | Meaning |
|---:|---|
| 0 | UNKNOWN |
| 1 | POWER_ON |
| 2 | SOFTWARE |
| 3 | WATCHDOG |
| 4 | UNDERVOLTAGE |
| 5 | OVERVOLTAGE |
| 6 | OVERTEMPERATURE |
| 7 | OTHER_PLATFORM |

When the platform exposes a more detailed PMIC reset reason, PI-MGMT maps it to this enum and may include the raw diagnostic string only in local logs.

### 5.3 Pi monitoring

Predeclared Service 12 definitions shall include at minimum:

- `pi5.soc_temp` high warning/critical monitoring;
- active undervoltage;
- active thermal throttling;
- payload/update storage low/critical;
- Camera/Antenna restart storm.

Temperature thresholds remain Service 20 configuration because cooling/environment changes the appropriate policy. Firmware thermal-throttling state is always treated as a health alarm independently of the configured numeric threshold.

## 6. AMD Kria K26 SOM profile — 0x0201

The K26 Zynq UltraScale+ MPSoC provides PS/PL System Monitor ADCs with on-die temperature and supply-voltage monitoring.

The EXN FPGA endpoint owns these measurements even if Linux on the A53 reads them.

### 6.1 Required K26 physical sensors

| Sensor ID | Name | Type/unit | Requirement | SYSMON source |
|---:|---|---|---|---|
| `0x2101` | `k26.junction_temp` | I32 cC | required | PS or PL on-die temperature sensor |
| `0x2110` | `k26.ps_0v85` | U32 mV | required | VCC_PSINTLP / VCC_PSINTFP domain |
| `0x2111` | `k26.ps_1v80` | U32 mV | required | VCC_PSAUX |
| `0x2120` | `k26.pl_0v72` | U32 mV | required | VCCINT |
| `0x2121` | `k26.pl_1v80` | U32 mV | required | VCCAUX |
| `0x2122` | `k26.pl_0v85` | U32 mV | required | VCCBRAM |

The local [FPGA implementation architecture](https://github.com/ExoSpaceLabs/exn-fpga-ai/blob/main/docs/IMPLEMENTATION_ARCHITECTURE.md) owns AMD monitoring driver selection and raw-value conversion; **the mission interface always transports the engineering values defined above**, not raw ADC codes.

If both PS and PL temperature channels are available, `k26.junction_temp` reports the higher valid junction-temperature measurement. An implementation may later expose the individual raw-domain temperatures using additional IDs if operationally useful.

### 6.2 K26 monitoring

Service 12 shall monitor:

- junction temperature;
- each required monitored rail against configured valid ranges;
- System Monitor alarm state where available.

The temperature policy must respect the actual SOM grade. For example, K26 commercial and industrial devices have different allowed junction-temperature ranges; the profile/board identity determines the configured limits rather than one global magic number.

## 7. KV260 carrier extension — 0x0202

KV260 adds carrier-level SOM power telemetry. The carrier includes an INA260 current/power monitor on the SOM 5 V rail at I2C address `0x40`.

These sensors are required for the KV260 reference profile:

| Sensor ID | Name | Type/unit | Requirement | Source |
|---:|---|---|---|---|
| `0x2180` | `kv260.som_5v_voltage` | U32 mV | required | INA260 bus voltage |
| `0x2181` | `kv260.som_5v_current` | U32 mA | required | INA260 current |
| `0x2182` | `kv260.som_5v_power` | U32 mW | required | INA260 power or voltage×current |

The K26 sensors from Section 6 remain present.

The [FPGA implementation](https://github.com/ExoSpaceLabs/exn-fpga-ai/blob/main/docs/IMPLEMENTATION_ARCHITECTURE.md) owns Linux/I2C monitoring acquisition details. Neither a driver path nor a vendor monitoring utility is normative to the PUS interface.

## 8. FPGA/Kria HEALTH extension

Useful non-sensor FPGA/Kria telemetry belongs in HEALTH:

```text
apu_cpu_load_permille:u16
memory_available_bytes:u64
pl_configured:u8
active_image_id:u32
active_capability_set_id:u32
active_jobs:u16
queued_jobs:u16
dma_error_count:u32
processing_error_count:u32
thermal_alarm_flags:u32
```

A future RPU/lock-step controller implementation may add RPU state without changing the physical sensor catalogue.

## 9. Capability and compatibility rules

1. Sensor IDs defined as **required** for the active reference profile shall be implemented before that hardware profile is declared EXN-ready.
2. Optional sensors are reported only when the target OS/driver/hardware exposes a trustworthy value.
3. Hardware-specific sensors do not migrate into another endpoint merely because two services share one computer. Pi board telemetry belongs to PI-MGMT; Kria board telemetry belongs to FPGA.
4. A derived engineering value is allowed when the derivation is defined, for example VDDA from VREFINT or KV260 power from voltage×current.
5. Unsupported measurements use `UNAVAILABLE`; zero is a real numeric measurement and shall not mean "not supported".
6. Service 12 threshold definitions reference these same sensor IDs.
7. Service 20 changes threshold/policy values; it does not change sensor identity or units.

## 10. Reference hardware evidence

The reference profiles are based on the capabilities of the actual selected hardware:

- STM32H755: internal temperature sensor, factory calibration, VREFINT and VBAT ADC paths.
- Raspberry Pi 5: kernel/firmware SoC-temperature reporting and PMIC ADC telemetry including `EXT5V_V`.
- AMD K26: integrated PS/PL System Monitor with junction-temperature and internal supply-voltage sensing.
- AMD KV260: K26 monitoring plus INA260 SOM 5 V power telemetry.

This document defines the EXN mission contract. Low-level Linux/DAS/AMD driver paths are implementation details and may change without changing the sensor IDs or engineering units.


## 11. Reference documents

The reference profile assignments above are grounded in the selected hardware documentation:

- STMicroelectronics STM32H755xI datasheet: internal calibrated temperature sensor, VREFINT and VBAT ADC paths.
- Raspberry Pi hardware documentation: thermal-zone temperature reporting, PMIC ADC telemetry including `EXT5V_V`, PMIC reset information and power-supply health.
- AMD Kria K26 SOM Data Sheet (DS987): integrated System Monitor, power-domain monitors and SOM platform management devices.
- AMD Kria SOM Carrier Card Design Guide (UG1091): K26 System Monitor coverage for PS/PL power rails.
- AMD Kria K26 Thermal Design Guide (UG1090): K26 junction-temperature monitoring and grade-specific thermal limits.
- AMD Kria KV260 Starter Kit Data Sheet/User Guide (DS986/UG1089): INA260-based SOM 5 V power telemetry and AMD platform telemetry utilities.

These external documents establish what the hardware can measure. The stable EXN sensor IDs, units, ownership and packet semantics are defined by this ICD.
