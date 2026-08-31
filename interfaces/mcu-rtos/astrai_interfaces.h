#ifndef ASTRAI_INTERFACES_COMPAT_H
#define ASTRAI_INTERFACES_COMPAT_H

/*
 * Deprecated compatibility include.
 * New EXN code should include exn_interfaces.h and use exn_* typedef names.
 */
#include "exn_interfaces.h"

typedef exn_apid_t astr_ai_apid_t;
typedef exn_srcid_t astr_ai_srcid_t;
typedef exn_service_t astr_ai_service_t;
typedef exn_subservice_t astr_ai_subservice_t;
typedef exn_result_t astr_ai_result_t;
typedef exn_tlv_type_t astr_ai_tlv_type_t;
typedef exn_pixel_t astr_ai_pixel_t;
typedef exn_proxy_preamble_t astr_ai_proxy_preamble_t;
typedef exn_hk_generic_t astr_ai_hk_generic_t;
typedef exn_sys_hk_req_t astr_ai_sys_hk_req_t;
typedef exn_sys_hk_tm_hdr_t astr_ai_sys_hk_tm_hdr_t;
typedef exn_cam_capture_tc_t astr_ai_cam_capture_tc_t;
typedef exn_ack_tm_t astr_ai_ack_tm_t;
typedef exn_xfer_meta_tm_t astr_ai_xfer_meta_tm_t;
typedef exn_xfer_chunk_tm_hdr_t astr_ai_xfer_chunk_tm_hdr_t;
typedef exn_xfer_done_tm_t astr_ai_xfer_done_tm_t;
typedef exn_gs_link_ack_tm_t astr_ai_gs_link_ack_tm_t;

#endif /* ASTRAI_INTERFACES_COMPAT_H */
