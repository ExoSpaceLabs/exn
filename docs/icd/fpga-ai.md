# FPGA-AI Device/Service ICD

This document specializes the EXN ICD for the FPGA Processing node (APID 0x102). It defines Housekeeping (HK) telemetry, processing settings (Service 210), execution control, and data/result transfer behavior.

Refer to `../ICD.md` for CCSDS/PUS conventions and the master service catalog.

---

## 1. Role and Interfaces
- Performs preprocessing (resize/normalize), hardware-accelerated inference, and postprocessing.
- Receives control and settings from MCU-RTOS and publishes results/telemetry.
- May receive image data directly from PI-CAM or from MCU-RTOS depending on system configuration.

Transports: SPI, UART, CAN, or UDP. Application data fields are big-endian. Timestamps use 6-byte CUC where present.

---

## 2. Housekeeping (Service 3)

Notes:
- FPGA-AI SHALL respond promptly to TC 3/1 (HK Request) issued by MCU. Its TM 3/2 payload below is included verbatim as the FPGA block inside the aggregated System HK TM 3/100 when GS requests System HK (TC 3/10).

- TM 3/2 Report HK — Application Data

| Name                  | Type   | Bits | Description |
|-----------------------|--------|------|-------------|
| uptime_ms             | uint64 | 64   | Node uptime in ms |
| fpga_temp_cC          | int16  | 16   | FPGA die temperature (centi-°C) |
| vccint_mV             | uint16 | 16   | Core voltage in millivolts |
| vccaux_mV             | uint16 | 16   | Aux voltage in millivolts |
| vccbram_mV            | uint16 | 16   | BRAM voltage in millivolts |
| fabric_clock_khz      | uint32 | 32   | Main fabric clock frequency (kHz) |
| dma_tx_pkts           | uint32 | 32   | DMA packets transmitted |
| dma_rx_pkts           | uint32 | 32   | DMA packets received |
| dma_rx_dropped        | uint32 | 32   | DMA dropped packets |
| last_infer_ms         | uint32 | 32   | Duration of last full pipeline (ms) |
| bram_used_bytes       | uint32 | 32   | BRAM usage bytes (estimate) |
| dsp_util_permille     | uint16 | 16   | DSP slice utilization (0..1000) |
| status_flags          | uint16 | 16   | Bitmask: [0]idle, [1]busy, [2]error, [3]overtemp, [4]throttle |
| last_error            | uint16 | 16   | Last error code |
| ts_cuc                | bytes  | 48   | 6-byte CUC timestamp |

---

## 3. Processing Control (Service 210)
### 3.1 Execute (TC 210/1)
Controls pipeline execution. If `flags.auto_start` is set, execution may start automatically when input is available.

- Application Data

| Field    | Type   | Bits | Description |
|----------|--------|------|-------------|
| pipeline | uint8  | 8    | 0=pre,1=infer,2=post,3=full |
| modelId  | uint16 | 16   | Model selection ID |
| flags    | uint16 | 16   | Bitfield: [0]auto_start, [1]async, [2]save_intermediate |

- TM 210/5 ACK/NACK — See `../ICD.md` (Service 210) for structure.

### 3.2 Processing Settings (TC 210/2 Set, TC 210/3 Get, TM 210/4 Report)
Use TLV per `../ICD.md`. TLV Types: 1=U8,2=U16,3=U32,4=I32,5=F32,6=STR,7=BYTES,8=U64,9=BOOL.

#### 3.2.1 Settings Catalog

| Key | Name                 | TLV | Units | Range/Enum | Default | Description |
|-----|----------------------|-----|-------|------------|---------|-------------|
| 1   | input.width          | U16 | px    | 16..8192   | 224     | Inference input width |
| 2   | input.height         | U16 | px    | 16..8192   | 224     | Inference input height |
| 3   | input.channels       | U8  | ch    | 1..4       | 3       | Number of channels |
| 4   | input.format         | U8  | enum  | 1=RGB888,2=GRAY8,3=GRAY16,4=YUV420 | 1 | Expected input format |
| 5   | resize.enable        | BOOL| bool  | 0/1        | 1       | Enable hardware resize |
| 6   | resize.algorithm     | U8  | enum  | 0=nearest,1=bilinear,2=bicubic | 1 | Resize method |
| 7   | norm.mean0           | F32 |       | -1..+1     | 0.485   | Channel 0 mean (RGB order) |
| 8   | norm.mean1           | F32 |       | -1..+1     | 0.456   | Channel 1 mean |
| 9   | norm.mean2           | F32 |       | -1..+1     | 0.406   | Channel 2 mean |
| 10  | norm.std0            | F32 |       | >0         | 0.229   | Channel 0 std |
| 11  | norm.std1            | F32 |       | >0         | 0.224   | Channel 1 std |
| 12  | norm.std2            | F32 |       | >0         | 0.225   | Channel 2 std |
| 13  | quant.enable         | BOOL| bool  | 0/1        | 0       | Enable quantization |
| 14  | quant.scale          | F32 |       | >0         | 1.0     | Quantization scale |
| 15  | quant.zero_point     | I32 |       | -128..127  | 0       | Zero point for INT8 |
| 16  | model.id             | U16 | id    | 0..65535   | 1       | Model selection |
| 17  | model.name           | STR |       |            | ""      | Optional model string name |
| 18  | pipeline.mode        | U8  | enum  | 0=pre,1=infer,2=post,3=full | 3 | Default pipeline mode |
| 19  | dma.burst_len        | U16 | beats | 1..256     | 64      | DMA burst length |
| 20  | dma.timeout_ms       | U16 | ms    | 0..60000   | 2000    | DMA transfer timeout |
| 21  | tile.width           | U16 | px    | 8..1024    | 224     | Tiling width for large images |
| 22  | tile.height          | U16 | px    | 8..1024    | 224     | Tiling height |
| 23  | output.format        | U8  | enum  | 0=raw,1=logits_f32,2=logits_i8,3=classes | 3 | Output result format |
| 24  | post.topk            | U8  | K     | 1..100     | 5       | Top-K classes reported |
| 25  | post.threshold       | F32 |       | 0..1       | 0.5     | Score threshold |

Notes:
- If `quant.enable=1` and `output.format=2`, logits are emitted as INT8 with `quant.scale` and `quant.zero_point` metadata.

---

## 4. Data/Result Transfer (Service 23)
- Input images may be received via TM 23/11 Data Chunk from PI-CAM (APID 0x101) if MCU configures path accordingly.
- Results are typically returned as TM 23/11 chunks to MCU with a job identifier.

### 4.1 Result Packets
When `output.format` is not `raw`, use the following application data conventions for result TMs (Service 23):

- TM 23/11 Data Chunk (Results)

| Name     | Type   | Bits | Description |
|----------|--------|------|-------------|
| jobId    | uint16 | 16   | Inference job identifier |
| offset   | uint32 | 32   | Byte offset in result buffer |
| data     | bytes  | var  | Encoded result bytes per `output.format` |

- TM 23/12 Transfer Complete (Results)

| Name        | Type   | Bits | Description |
|-------------|--------|------|-------------|
| jobId       | uint16 | 16   | Job identifier |
| resultCode  | uint8  | 8    | 0=OK, >0 error |

If `output.format=3 (classes)`, the first chunk SHOULD begin with a header: `numClasses(U16)`, followed by repeated entries `{classId(U16), score(F32)}`.

---

## 5. Parameter Management (Service 20)
Generic TLV parameters are supported for experimental configuration (keys 100..199 reserved for FPGA-specific extras not covered by Service 210).

---

## 6. CCSDSPack Interfaces
- `pkt_fpga_exec_tc`
- `pkt_xfer_chunk_tm` (results), `pkt_xfer_done_tm` (results)
Interfaces set APID=0x102 and appropriate PUS-A fields per service/subservice.
