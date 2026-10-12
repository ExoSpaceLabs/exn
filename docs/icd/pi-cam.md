# PI-CAM Device/Service ICD — Raspberry Pi 5 + Camera Module 3

PI-CAM is the EXN Camera endpoint on the Raspberry Pi 5 Payload Node.

- APID: `0x101`
- Target PUS-C source identity: `0x02`
- Reference sensor: Raspberry Pi Camera Module 3 / Sony IMX708
- Mission role: capture, preserve, derive, catalog and transfer image products

The same physical Pi also hosts the Antenna endpoint (PI-COMMS) and PI-MGMT supervisor. Those are separate logical endpoints.

Refer to:

- `../component-services.md` for the common functional contract;
- `../services.md` for target PUS mapping;
- `../ICD.md` for packet/routing rules;
- `linux-payload.md` for Pi supervision and Antenna behavior.

The currently executable PUS-A camera packets remain transitional compatibility artifacts. New Camera implementation work targets this document.

## 1. Reference-camera capabilities

The reference Camera Module 3 provides:

- Sony IMX708 sensor;
- native resolution up to 4608x2592 in the normal non-HDR mode;
- RAW10 sensor output;
- powered autofocus;
- manual, triggered-auto and continuous autofocus through the current libcamera/Picamera2 stack;
- HDR support subject to sensor/mode restrictions.

EXN does **not** hard-code driver ranges where libcamera can report the actual supported control metadata. The runtime Camera Capabilities Report is authoritative for:

- available sensor modes;
- min/max/default values;
- supported autofocus controls;
- exposure/gain limits;
- frame-rate constraints;
- supported transforms and image formats.

## 2. Service set

PI-CAM provides/consumes:

- Service 1 Request Verification;
- Service 3 Housekeeping;
- Service 5 Event Reporting;
- Service 8 Function Management for Camera reinitialization/idle operations;
- Service 9 Time Management;
- Service 12 On-board Monitoring;
- Service 20 Parameter Management for generic service/storage policy;
- Service 200 Camera Control;
- Service 220 Payload Object Transfer.

Software activation/restart of the Camera process is owned by PI-MGMT through Service 230/Service 8.

## 3. Camera state

PI-CAM detailed state is one of:

| State | Meaning |
|---|---|
| `STARTING` | service/camera initialization |
| `READY` | camera available, no acquisition active |
| `ACQUIRING` | finite acquisition in progress |
| `STREAMING` | continuous acquisition active |
| `PROCESSING` | deriving/storing requested product formats |
| `DEGRADED` | responsive with reduced camera/storage capability |
| `ERROR` | camera function unavailable |
| `IDLE` | deliberately quiesced |

These map to the common OBC endpoint states.

## 4. Service 200 message catalogue

| Message | Name | Purpose |
|---|---|---|
| `TC[200,1]` | Start Acquisition | create single/burst/timelapse/stream acquisition |
| `TC[200,2]` | Stop Acquisition | stop active finite/stream acquisition |
| `TC[200,3]` | Set Camera Controls | set one or more camera controls atomically where possible |
| `TC[200,4]` | Get Camera Controls | request current control values |
| `TM[200,5]` | Camera Controls Report | current/read-back controls |
| `TC[200,6]` | Get Camera Capabilities | request available controls/modes/ranges |
| `TM[200,7]` | Camera Capabilities Report | capabilities and dynamic ranges |
| `TC[200,8]` | List Products | query stored Camera products |
| `TM[200,9]` | Product Catalog Report | product/object metadata |
| `TC[200,10]` | Delete Product | delete one or more locally stored products |
| `TC[200,11]` | Get Acquisition Status | request acquisition/session status |
| `TM[200,12]` | Acquisition Status Report | state/progress/product IDs |

All state-changing TCs use Service 1 acceptance/completion verification.

## 5. Acquisition control

### 5.1 Start Acquisition

`TC[200,1]` application fields:

| Field | Type | Meaning |
|---|---|---|
| `acquisitionId` | uint32 | OBC-assigned acquisition/session ID |
| `mode` | uint8 | SINGLE/BURST/TIMELAPSE/STREAM |
| `count` | uint16 | requested frame count; 1 for SINGLE |
| `interval_ms` | uint32 | interval for burst/timelapse; zero = fastest valid |
| `duration_ms` | uint32 | optional bounded stream/session duration; zero = not duration-limited |
| `product_mask` | uint16 | products to retain/derive |
| `flags` | uint16 | acquisition policy flags |

Modes:

| Value | Mode | Baseline |
|---:|---|---|
| 0 | SINGLE | required |
| 1 | BURST | required |
| 2 | TIMELAPSE | required |
| 3 | STREAM | optional/future-capable |

STREAM is intentionally represented now so later live/video work does not require replacing the command model.

### 5.2 Stop Acquisition

`TC[200,2]` contains `acquisitionId`.

For finite capture modes, Stop cancels remaining frames. For STREAM it terminates the stream/session cleanly.

## 6. Camera controls

`TC[200,3]` and `TC[200,4]` operate on stable EXN control IDs. Actual support/range is reported by `TM[200,7]`.

### 6.1 Sensor/output geometry

| Control | Meaning |
|---|---|
| `sensor.mode` | selected sensor/libcamera mode |
| `output.width` | output width |
| `output.height` | output height |
| `output.pixel_format` | application pixel format |
| `frame_rate` | requested frame rate |
| `crop.x/y/w/h` | crop/ROI |
| `transform.hflip` | horizontal flip |
| `transform.vflip` | vertical flip |

Rotation requiring a host transform is represented as product-processing policy rather than assumed to be a sensor control.

### 6.2 Exposure / gain

| Control | Meaning |
|---|---|
| `ae.enable` | automatic exposure/gain control |
| `exposure_us` | exposure time when manual/overridden |
| `analogue_gain` | sensor analogue gain |
| `exposure_compensation_ev` | AE compensation |
| `metering_mode` | metering algorithm when supported |
| `exposure_mode` | normal/short/long/custom mode where supported |

Camera Module 3 supports long exposures, but exact active-mode limits are obtained from the runtime capabilities rather than encoded as mission constants.

### 6.3 Autofocus

| Control | Meaning |
|---|---|
| `af.mode` | MANUAL / AUTO / CONTINUOUS |
| `af.trigger` | START / CANCEL for triggered autofocus |
| `af.range` | supported AF search-range policy |
| `af.speed` | supported AF speed policy |
| `lens.position_dioptres` | manual focus position; 0 represents infinity |
| `af.window` | normalized/declared autofocus region where supported |

AF status metadata includes current AF state and read-back lens position.

### 6.4 White balance / colour / ISP controls

| Control | Meaning |
|---|---|
| `awb.enable` | automatic white balance |
| `awb.mode` | AWB profile when supported |
| `awb.red_gain` | manual red gain |
| `awb.blue_gain` | manual blue gain |
| `brightness` | image brightness |
| `contrast` | image contrast |
| `saturation` | image saturation |
| `sharpness` | sharpening |
| `denoise.mode` | denoise policy |
| `hdr.enable` | sensor/HDR mode request where supported |

### 6.5 Product controls

| Control | Meaning |
|---|---|
| `jpeg.quality` | JPEG encoder quality |
| `thumbnail.enable` | create preview product |
| `thumbnail.max_width` | preview bound |
| `storage.retention_count` | local retention policy |
| `storage.minimum_free_bytes` | guard before accepting capture |

## 7. Product generation and storage

Each captured source frame has a `captureId`. One capture may generate several stored products.

### 7.1 Product mask

| Bit | Product | Notes |
|---:|---|---|
| 0 | SENSOR_RAW | raw/Bayer sensor data |
| 1 | DNG | raw sensor data in DNG where supported |
| 2 | TIFF | lossless derived still |
| 3 | JPEG | compressed still |
| 4 | PNG | lossless host-encoded still |
| 5 | THUMBNAIL | preview/quick-look |
| 6 | STREAM_ARTIFACT | future video/stream output |

When RAW is requested, PI-CAM shall preserve that source product before deriving TIFF/JPEG/other formats. Derivation never silently replaces the source.

### 7.2 Product metadata

Every stored product has an opaque `objectId:uint64`.

Catalog metadata includes:

- `objectId`;
- `acquisitionId`;
- `captureId`;
- `created_unix_ms`;
- product type/format;
- width/height;
- pixel format/bit depth;
- byte size;
- checksum;
- parent/source object ID where derived;
- storage state;
- transfer state if active.

The mission interface does not expose Linux filesystem paths as the object identity.

### 7.3 Product lifecycle

PI-CAM owns:

- creation;
- validation;
- retention;
- cataloging;
- explicit deletion;
- garbage collection according to configured storage policy.

The OBC may list products before deciding whether to process, retain, transfer to OBC, transfer to FPGA, or downlink to Ground.

## 8. Payload transfer

Service 220 transfers Camera objects.

Examples:

```text
OBC -> PI-CAM: transfer RAW object to FPGA
PI-CAM -> FPGA

OBC -> PI-CAM: downlink JPEG object to Ground
PI-CAM -> Antenna -> Ground

OBC -> PI-CAM: transfer TIFF object to OBC
PI-CAM -> OBC
```

PI-CAM never opens the Ground link directly. A Ground destination resolves through the Antenna service.

Because Camera and Antenna are co-located, the local Pi implementation may optimize the PI-CAM -> Antenna bulk path with file/object references, shared memory or local IPC while preserving Service 220 semantics externally.

## 9. Housekeeping

PI-CAM exposes predeclared Service 3 structures.

### HEARTBEAT

- Camera state;
- software version;
- acquisition/session state;
- active acquisition ID;
- mission time sync state;
- uptime;
- last error;
- restart count.

### HEALTH

- camera present/available;
- storage available/free bytes for Camera product store;
- frames captured;
- frames failed/dropped;
- active product-derivation jobs;
- last capture duration;
- last encode duration;
- control/read-back validity.

### SENSORS / runtime metadata

- camera/sensor temperature **only if the active driver exposes a valid value**;
- current exposure time;
- current analogue gain;
- AF state;
- lens position;
- relevant sensor metadata exposed by the selected libcamera mode.

Host-wide SoC temperature/memory/filesystem/platform state belongs primarily to PI-MGMT, not duplicated as Camera ownership.

## 10. Monitoring and events

PI-CAM supports predeclared Service 12 monitoring for Camera-owned health parameters such as:

- repeated capture failure;
- storage-low threshold;
- frame-drop/error rate;
- camera unavailable/disconnected;
- sensor temperature when available.

Representative Service 5 events include:

- Camera service ready;
- acquisition started/completed/cancelled;
- product created/deleted;
- Camera unavailable;
- AF failure;
- storage low/critical;
- product encoding failure;
- repeated frame capture failure.

Threshold values and severity mapping are configured rather than buried in code.

## 11. Time

PI-CAM consumes OBC `TC[9,128] Set Mission Time` and timestamps acquisitions/products using synchronized `unix_ms`.

Each product records capture time from the EXN mission clock independently from Linux filesystem timestamps.

## 12. Recovery

PI-CAM itself may provide a Service 8 reinitialize-camera function while still responsive.

Process-level restart is commanded:

```text
OBC -> PI-MGMT: restart exn-camera.service
```

This remains functional even when PI-CAM is unresponsive.

## 13. Software update

PI-CAM packages are transferred/staged and activated by PI-MGMT using Service 220 + Service 230.

Activation shall restart only the Camera service unless the package declares a platform dependency requiring Pi reboot.

Post-update success requires Camera heartbeat, Camera presence and basic capability report to succeed.
