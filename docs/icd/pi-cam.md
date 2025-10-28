# PI-CAM Device/Service ICD — Raspberry Pi 5 + Camera Module 3

This document specializes the ASTRAi ICD for the Raspberry Pi 5 camera node (APID 0x101) equipped with the Raspberry Pi Camera Module 3. It defines the node’s Housekeeping (HK) telemetry, camera settings catalog (Service 200), and data transfer behavior (Service 23).

Refer to `../ICD.md` for CCSDS/PUS conventions, primary/secondary header definitions, and the master service catalog.

---

## 1. Role and Interfaces
- Captures images from Camera Module 3 and publishes metadata and data chunks.
- Responds to TCs from MCU-RTOS for capture, settings, parameter management, and transfer control.
- Can send data to MCU-RTOS and/or FPGA-AI depending on TC `dest`.

Transports: SPI, UART, CAN, or UDP as deployed. Application data follows big-endian unless otherwise stated. Timestamps use 6-byte CUC by default.

---

## 2. Housekeeping (Service 3)
The PI-CAM HK report augments the generic HK in `ICD.md` with camera- and SoC-specific metrics. Values are big-endian.

Notes:
- PI-CAM SHALL respond promptly to TC 3/1 (HK Request) issued by MCU. Its TM 3/2 payload below is included verbatim as the PI block inside the aggregated System HK TM 3/100 when GS requests System HK (TC 3/10).

- TM 3/2 Report HK — Application Data

| Name                | Type   | Bits | Description |
|---------------------|--------|------|-------------|
| uptime_ms           | uint64 | 64   | Node uptime in ms |
| soc_temp_cC         | int16  | 16   | SoC temperature (centi-°C) |
| cam_temp_cC         | int16  | 16   | Camera sensor temperature (centi-°C), 0x7FFF if unknown |
| throttling_flags    | uint16 | 16   | Bitmask: [0]undervolt, [1]freq_capped, [2]throttled, [3]temp_limit, [4]soft_oc |
| cpu_load_mpermil    | uint16 | 16   | CPU load in milli‑percent (0..100000) |
| mem_used_kib        | uint32 | 32   | Used memory kibibytes |
| mem_free_kib        | uint32 | 32   | Free memory kibibytes |
| frames_captured     | uint32 | 32   | Total frames captured since boot |
| frames_dropped      | uint32 | 32   | Frames dropped (pipeline) |
| last_capture_ms     | uint32 | 32   | Duration of last capture (ms) |
| isp_analogue_gain_q8| uint16 | 16   | ISP analogue gain in Q8.8 (divide by 256) |
| isp_digital_gain_q8 | uint16 | 16   | ISP digital gain in Q8.8 |
| exposure_us         | uint32 | 32   | Current exposure time (µs) |
| iso                 | uint16 | 16   | Current ISO (100..1600 typical) |
| wb_r_gain_q8        | uint16 | 16   | AWB red gain Q8.8 |
| wb_b_gain_q8        | uint16 | 16   | AWB blue gain Q8.8 |
| lens_position       | uint16 | 16   | Lens focus position (0..1023), 0xFFFF if N/A |
| status_flags        | uint16 | 16   | Bitmask: [0]cam_present, [1]streaming, [2]recording, [3]af_locked, [4]ae_locked |
| last_error          | uint16 | 16   | Last error code |
| ts_cuc              | bytes  | 48   | 6-byte CUC timestamp |

---

## 3. Camera Control (Service 200)
Service 200 defines camera‑specific TCs/TMs. The set aligns with the master catalog but enumerates concrete keys, types, units, and ranges for Camera Module 3.

- TC 200/1 Capture — Application Data

| Field        | Type   | Bits | Values |
|--------------|--------|------|--------|
| mode         | uint8  | 8    | 0=single, 1=burst |
| burst_count  | uint16 | 16   | 1..1000 (ignored if mode=single) |
| exposure_us  | uint32 | 32   | 0=auto; else manual exposure time in µs |

- TM 200/5 ACK/NACK — as in `ICD.md` Section 4.4.

- TC 200/2 Settings Set and TC 200/3 Settings Get use the key/TLV catalog below.
- TM 200/4 Settings Report returns `key`, `status`, `value` (TLV) for the requested key or the last set.

### 3.1 TLV Types
Use these TLV type codes in Application Data:
- 1=U8, 2=U16, 3=U32, 4=I32, 5=F32 (IEEE‑754), 6=STR (UTF‑8), 7=BYTES, 8=U64, 9=BOOL (U8 0/1).

### 3.2 Camera Settings Catalog (Camera Module 3)
Keys are stable 8‑bit IDs. Unless noted, setting takes effect on next capture/start.

| Key | Name                  | TLV | Units   | Range/Enum | Default | Description |
|-----|-----------------------|-----|---------|------------|---------|-------------|
| 1   | sensor.mode           | U8  | enum    | 0=auto,1=still,2=video | 0 | Sensor operating preset |
| 2   | stream.enable         | BOOL| bool    | 0/1        | 0       | Continuous streaming mode |
| 3   | size.width            | U16 | px      | 64..4608   | 1920    | Output width |
| 4   | size.height           | U16 | px      | 64..2592   | 1080    | Output height |
| 5   | framerate             | F32 | fps     | 0.1..120.0 | 30.0    | Frames per second |
| 6   | pixel.format          | U8  | enum    | 1=RGB888,2=GRAY8,3=GRAY16,4=YUV420,5=BAYER_RGGB8 | 1 | Output pixel format |
| 7   | bayer.order           | U8  | enum    | 0=RGGB,1=BGGR,2=GRBG,3=GBRG | 0 | Sensor Bayer order (raw modes) |
| 8   | stride.align          | U16 | bytes   | 1..256     | 64      | Line stride alignment |
| 9   | roi.x                 | U16 | px      | 0..(W-1)   | 0       | ROI top-left X |
| 10  | roi.y                 | U16 | px      | 0..(H-1)   | 0       | ROI top-left Y |
| 11  | roi.w                 | U16 | px      | 1..W       | W       | ROI width |
| 12  | roi.h                 | U16 | px      | 1..H       | H       | ROI height |
| 13  | flip.h                | BOOL| bool    | 0/1        | 0       | Horizontal flip |
| 14  | flip.v                | BOOL| bool    | 0/1        | 0       | Vertical flip |
| 15  | rotate.deg            | U16 | deg     | 0,90,180,270 | 0     | Rotation |
| 16  | ae.enable             | BOOL| bool    | 0/1        | 1       | Auto Exposure enable |
| 17  | ae.exposure_us        | U32 | µs      | 100..33000 | 0       | Manual exposure; 0=auto |
| 18  | ae.iso                | U16 | ISO     | 100..1600  | 0       | ISO; 0=auto |
| 19  | ae.metermode          | U8  | enum    | 0=center,1=spot,2=matrix | 2 | Metering mode |
| 20  | ae.comp_ev            | F32 | EV      | -4.0..+4.0 | 0.0     | Exposure compensation |
| 21  | af.mode               | U8  | enum    | 0=off,1=auto,2=continuous | 1 | Autofocus mode (CM3 supports PDAF) |
| 22  | af.trigger            | U8  | enum    | 0=none,1=start,2=cancel | 0 | One‑shot AF trigger |
| 23  | af.range              | U8  | enum    | 0=normal,1=macro,2=full | 0 | AF range |
| 24  | lens.position         | U16 | ticks   | 0..1023    | 0xFFFF  | Manual focus position; 0xFFFF=auto |
| 25  | awb.enable            | BOOL| bool    | 0/1        | 1       | Auto White Balance enable |
| 26  | awb.mode              | U8  | enum    | 0=auto,1=incandescent,2=fluorescent,3=daylight,4=cloudy | 0 | WB preset |
| 27  | awb.r_gain            | F32 | gain    | 0.5..8.0   | 1.0     | Manual red gain (when awb.enable=0) |
| 28  | awb.b_gain            | F32 | gain    | 0.5..8.0   | 1.0     | Manual blue gain (when awb.enable=0) |
| 29  | denoise               | U8  | enum    | 0=off,1=low,2=med,3=high | 1 | Spatial/temporal NR level |
| 30  | sharpness             | F32 | factor  | 0.0..2.0   | 1.0     | Sharpen strength |
| 31  | contrast              | F32 | factor  | 0.0..2.0   | 1.0     | Contrast adjustment |
| 32  | saturation            | F32 | factor  | 0.0..2.0   | 1.0     | Saturation adjustment |
| 33  | brightness            | F32 | factor  | -1.0..1.0  | 0.0     | Brightness offset |
| 34  | hdr.enable            | BOOL| bool    | 0/1        | 0       | Enable sensor HDR (if supported) |
| 35  | hdr.merge             | U8  | enum    | 0=linear,1=reinhard,2=aces | 0 | HDR merge method |
| 36  | output.jpeg_quality   | U8  | 0..100  | 0..100     | 90      | JPEG encode quality (if JPEG) |
| 37  | output.compress       | U8  | enum    | 0=none,1=JPEG,2=LZ4      | 0 | App‑level compression |
| 38  | xfer.chunk_size       | U16 | bytes   | 256..4096  | 900     | Preferred chunk size for Service 23 |
| 39  | xfer.dest             | U8  | enum    | 0=FPGA,1=MCU | 1      | Default data transfer destination |
| 40  | ts.embed_mode         | U8  | enum    | 0=none,1=app_payload,2=pus_c | 2 | Where timestamps are placed |

Notes:
- Changing resolution/framerate may reconfigure the sensor pipeline and briefly stall.
- Some ranges depend on mode; Camera Module 3 supports up to 4608×2592 stills, high-speed modes at reduced FOV.

---

## 4. Data Transfer (Service 23)
- TM 23/10 Metadata includes image dimensions, pixel type, total size, and chunk size per `../ICD.md`.
- TM 23/11 Data Chunk uses `imageId`, `offset`, and `data[]` up to `xfer.chunk_size`.
- TM 23/12 Transfer Complete summarizes the chunk count. Sequence flags typically UNSEGMENTED; app-level `offset` provides ordering.

---

## 5. Parameter Management (Service 20)
For generic parameter exchange not covered by Service 200, PI-CAM supports the Service 20 TLV scheme with keys reserved [200..255] for camera‑specific experimental features.

---

## 6. CCSDSPack Interfaces
- `pkt_cam_capture_tc`, `pkt_cam_ack_tm`
- `pkt_xfer_meta_tm`, `pkt_xfer_chunk_tm`, `pkt_xfer_done_tm`
Interfaces set Primary Header Type (TM/TC) and APID=0x101, with PUS-A secondary headers matching service/subservice.
