# PI-CAM Device/Service ICD — Raspberry Pi 5 + Camera Module 3

PI-CAM is the EXN Camera endpoint on the Raspberry Pi 5 Payload Node.

- APID: `0x101`
- PUS-C source identity: `0x0002`
- Reference sensor: Raspberry Pi Camera Module 3 / Sony IMX708
- Mission role: capture, preserve, derive, catalog and transfer image products

The same physical Pi also hosts the Antenna endpoint (PI-COMMS) and PI-MGMT supervisor. Those are separate logical endpoints.

Refer to:

- `../component-services.md` for the common functional contract;
- `../services.md` for the normative PUS-C service mapping;
- `../ICD.md` for packet/routing rules;
- `linux-payload.md` for Pi supervision and Antenna behavior.

All maintained PI-CAM packet handling shall use the PUS-C contract defined by this ICD. PUS-A Camera packet definitions are obsolete.

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
| `TC[200,3]` | Set Camera Configuration | set one or more camera controls atomically where possible |
| `TC[200,4]` | Get Camera Configuration | request current control values |
| `TM[200,5]` | Camera Configuration Report | current/read-back controls |
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

## 6. Camera configuration

Camera configuration is intentionally **batched**. EXN does not allocate a different TC for exposure, autofocus, gain, crop, white balance, JPEG quality, and similar settings.

`TC[200,3] Set Camera Configuration` carries a bounded set of configuration entries and applies them as one logical transaction. `TC[200,4]` requests the active configuration by scope; `TM[200,5]` reports it.

Actual control support/range is reported by `TM[200,7]`.

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

### 6.5 Encoding/product controls

| Control | Meaning |
|---|---|
| `jpeg.quality` | JPEG encoder quality |
| `thumbnail.enable` | create preview product |
| `thumbnail.max_width` | preview bound |

Storage retention and minimum-free-space policy are **not Camera image controls**. They are PI-CAM Service 20 runtime configuration parameters.

## 6.6 Stable control IDs and encoding

Camera control IDs are 16-bit and stable across implementations. Capability reports declare whether an ID is supported by the active Camera Module/libcamera stack.

| ID | Name | Type | Notes |
|---:|---|---|---|
| `0x0001` | `sensor.mode` | U16 | capability-reported sensor mode ID |
| `0x0002` | `output.width` | U16 | pixels |
| `0x0003` | `output.height` | U16 | pixels |
| `0x0004` | `output.pixel_format` | U8 | capability-reported EXN format enum |
| `0x0005` | `frame_rate` | F32 | frames/s |
| `0x0006` | `crop.x` | U16 | pixels |
| `0x0007` | `crop.y` | U16 | pixels |
| `0x0008` | `crop.w` | U16 | pixels |
| `0x0009` | `crop.h` | U16 | pixels |
| `0x000A` | `transform.hflip` | BOOL | |
| `0x000B` | `transform.vflip` | BOOL | |
| `0x0100` | `ae.enable` | BOOL | |
| `0x0101` | `exposure_us` | U32 | current/requested exposure |
| `0x0102` | `analogue_gain` | F32 | |
| `0x0103` | `exposure_compensation_ev` | F32 | |
| `0x0104` | `metering_mode` | U8 | capability-reported enum |
| `0x0105` | `exposure_mode` | U8 | capability-reported enum |
| `0x0200` | `awb.enable` | BOOL | |
| `0x0201` | `awb.mode` | U8 | capability-reported enum |
| `0x0202` | `awb.red_gain` | F32 | |
| `0x0203` | `awb.blue_gain` | F32 | |
| `0x0300` | `af.mode` | U8 | 0=MANUAL, 1=AUTO, 2=CONTINUOUS |
| `0x0301` | `af.trigger` | U8 | 1=START, 2=CANCEL; command-like/write-only |
| `0x0302` | `af.range` | U8 | capability-reported enum |
| `0x0303` | `af.speed` | U8 | capability-reported enum |
| `0x0304` | `lens.position_dioptres` | F32 | 0 = infinity |
| `0x0305` | `af.window.x` | U16 | normalized 0..65535 |
| `0x0306` | `af.window.y` | U16 | normalized 0..65535 |
| `0x0307` | `af.window.w` | U16 | normalized 0..65535 |
| `0x0308` | `af.window.h` | U16 | normalized 0..65535 |
| `0x0400` | `hdr.enable` | BOOL | |
| `0x0401` | `brightness` | F32 | driver range |
| `0x0402` | `contrast` | F32 | driver range |
| `0x0403` | `saturation` | F32 | driver range |
| `0x0404` | `sharpness` | F32 | driver range |
| `0x0405` | `denoise.mode` | U8 | capability-reported enum |
| `0x0500` | `jpeg.quality` | U8 | 1..100 |
| `0x0501` | `thumbnail.enable` | BOOL | |
| `0x0502` | `thumbnail.max_width` | U16 | pixels |

### 6.7 Configuration scopes

Scopes are used by `TC[200,4]` / `TM[200,5]` to request/report coherent groups without assigning new subservices.

| Bit | Scope |
|---:|---|
| 0 | sensor/output geometry |
| 1 | exposure/gain |
| 2 | white balance / colour |
| 3 | focus/autofocus |
| 4 | ISP/HDR/image processing |
| 5 | encoding/product |
| 6..15 | reserved |

`scope_mask=0xFFFF` means **all supported Camera configuration**.

### 6.8 Set Camera Configuration payload

`TC[200,3]`:

```text
flags:u16
count:u16
repeat count:
    control_id:u16
    value_type:u8
    value_length:u16
    value[value_length]
```

Configuration flags:

| Bit | Meaning |
|---:|---|
| 0 | `VALIDATE_ONLY` — validate complete batch, do not apply |
| 1 | `DEFER_UNTIL_IDLE` — if acquisition is active, stage until Camera becomes idle |
| 2 | `REQUIRE_EXACT` — fail rather than clamp/adapt a requested value |
| 3..15 | reserved |

**Atomicity rule:** the complete batch is validated first. If any requested entry is unsupported, invalid, conflicts with another entry, or cannot satisfy `REQUIRE_EXACT`, the configuration update fails and the active configuration is unchanged.

If the underlying Camera API cannot apply a requested multi-control update coherently, PI-CAM shall stage the values and apply them at a safe configuration boundary or reject the request. Partial silent application is not allowed.

Typical one-TC image setup therefore looks like:

```text
TC[200,3]
    output.width = 4608
    output.height = 2592
    ae.enable = false
    exposure_us = 12000
    analogue_gain = 2.0
    af.mode = AUTO
    awb.enable = true
    hdr.enable = false
    jpeg.quality = 92
```

One Service 1 acceptance/completion sequence verifies the whole transaction.

### 6.9 Get/Report Camera Configuration

`TC[200,4]`:

```text
scope_mask:u16       # 0xFFFF = all
```

`TM[200,5]`:

```text
configuration_revision:u32
scope_mask:u16
count:u16
repeat count:
    control_id:u16
    status:u8
    value_type:u8
    value_length:u16
    value[value_length]
```

`configuration_revision` increments after every successful applied configuration transaction and allows Ground/OBC to detect a changed Camera setup without comparing every field.

Entry status:

| Value | Meaning |
|---:|---|
| 0 | `OK` |
| 1 | `UNSUPPORTED` |
| 2 | `UNAVAILABLE_IN_CURRENT_MODE` |
| 3 | `READ_ONLY` |
| 4 | `INVALID` |

A report for `scope_mask=0xFFFF` is the complete active image configuration snapshot.

### 6.10 Capabilities report

`TM[200,7]` contains:

- Camera identity/model string with bounded length;
- active driver/libcamera version;
- supported sensor modes;
- supported product types;
- repeated control descriptors:
  - control ID;
  - value type;
  - access flags READ/WRITE/TRIGGER;
  - minimum;
  - maximum;
  - default;
  - current support status.

Variable-size strings/arrays shall have explicit bounded lengths. No Camera implementation may require unbounded dynamic packet fields.

## 6.11 PI-CAM Service 20 policy parameters

Service 20 is used only for Camera-service runtime/persistent **policy**, not image settings.

Initial PI-CAM parameters:

| ID | Name | Type | Meaning |
|---:|---|---|---|
| `0x1001` | `storage.retention_count` | U32 | local product-retention count; 0 = policy-defined/unbounded by count |
| `0x1002` | `storage.minimum_free_bytes` | U64 | reject new capture before crossing this free-space guard |
| `0x1003` | `catalog.max_entries_per_report` | U16 | bounded catalog page size |

Exposure, gain, focus, crop, colour, HDR and encoder settings remain Service 200 Camera configuration.

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

### 7.4 Product catalog messages

`TC[200,8] List Products` contains:

```text
filter_flags:u16
after_object_id:u64
max_entries:u16
```

`TM[200,9] Product Catalog Report` contains a bounded number of entries:

```text
count:u16
repeat count:
    object_id:u64
    acquisition_id:u32
    capture_id:u64
    created_unix_ms:u64
    product_type:u16
    width:u16
    height:u16
    byte_size:u64
    checksum_type:u8
    checksum[32]        # SHA-256 in baseline
    state:u8
```

Additional format-specific metadata is obtained from Service 220 Object Metadata when the object is transferred.

`TC[200,10] Delete Product` contains a bounded list of object IDs. Deletion fails for an object that is locked by an active acquisition/derivation/transfer unless an explicit force policy is later introduced.

### 7.5 Acquisition status

`TM[200,12]` contains:

```text
acquisition_id:u32
mode:u8
state:u8
requested_count:u16
captured_count:u16
failed_count:u16
product_count:u16
last_capture_id:u64
started_unix_ms:u64
updated_unix_ms:u64
last_error:u16
```

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
