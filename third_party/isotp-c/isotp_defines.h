#ifndef ISOTPC_USER_DEFINITIONS_H
#define ISOTPC_USER_DEFINITIONS_H

#include <stdint.h>

#include "isotp_config.h"

/**************************************************************
 * compiler specific defines
 *************************************************************/
#ifdef __GNUC__
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define ISOTP_BYTE_ORDER_LITTLE_ENDIAN
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #else
        #error "unsupported byte ordering"
    #endif

    #define ISOTP_PACKED_STRUCT(content) typedef struct __attribute__((packed)) content
#endif

/**************************************************************
 * OS specific defines
 *************************************************************/
#ifdef _MSC_VER
    #define ISOTP_PACKED_STRUCT(content) __pragma(pack(push, 1)) typedef struct content __pragma(pack(pop))

    #define snprintf _snprintf

    #include <windows.h>
    #define ISOTP_BYTE_ORDER_LITTLE_ENDIAN
    #define __builtin_bswap8 _byteswap_uint8
    #define __builtin_bswap16 _byteswap_uint16
    #define __builtin_bswap32 _byteswap_uint32
    #define __builtin_bswap64 _byteswap_uint64
#endif

#define LE32TOH(le) ((uint32_t)(((le) << 24) | (((le) & 0x0000FF00) << 8) | (((le) & 0x00FF0000) >> 8) | ((le) >> 24)))

/**************************************************************
 * CAN frame length (CAN_DL) defines
 *************************************************************/

/* The amount of data bytes a Classical CAN frame is able to carry */
#define ISOTP_CAN_DL_CLASSIC 8

#if (ISO_TP_MAX_CAN_FRAME_SIZE != 8) && (ISO_TP_MAX_CAN_FRAME_SIZE != 12) && (ISO_TP_MAX_CAN_FRAME_SIZE != 16) && (ISO_TP_MAX_CAN_FRAME_SIZE != 20) && \
    (ISO_TP_MAX_CAN_FRAME_SIZE != 24) && (ISO_TP_MAX_CAN_FRAME_SIZE != 32) && (ISO_TP_MAX_CAN_FRAME_SIZE != 48) && (ISO_TP_MAX_CAN_FRAME_SIZE != 64)
    #error "ISO_TP_MAX_CAN_FRAME_SIZE must be one of 8, 12, 16, 20, 24, 32, 48, 64"
#endif

#if (ISO_TP_DEFAULT_TX_DL > ISO_TP_MAX_CAN_FRAME_SIZE) || (ISO_TP_DEFAULT_TX_DL < ISOTP_CAN_DL_CLASSIC)
    #error "ISO_TP_DEFAULT_TX_DL must be at least 8 and must not exceed ISO_TP_MAX_CAN_FRAME_SIZE"
#endif

/* Flags describing the format a frame has to be transmitted with. These are passed
 * to isotp_user_send_can() if ISO_TP_USER_SEND_CAN_FLAGS is enabled.
 */
#define ISOTP_CAN_FRAME_FLAG_NONE 0x00 /* transmit as a Classical CAN frame */
#define ISOTP_CAN_FRAME_FLAG_FD 0x01   /* transmit as a CAN FD frame */
#define ISOTP_CAN_FRAME_FLAG_BRS 0x02  /* transmit using the data phase bit rate (bit rate switch) */

/* Largest payload which fits into a single frame, depending on the CAN frame
 * length (CAN_DL) in use. Frames larger than 8 bytes use the SF_DL escape
 * sequence, which occupies an additional byte of protocol control information.
 */
#define ISOTP_SF_MAX_PAYLOAD(can_dl) ((can_dl) > ISOTP_CAN_DL_CLASSIC ? (uint32_t)((can_dl) - 2u) : (uint32_t)((can_dl) - 1u))

/**************************************************************
 * internal used defines
 *************************************************************/
#define ISOTP_RET_OK 0
#define ISOTP_RET_ERROR -1
#define ISOTP_RET_INPROGRESS -2
#define ISOTP_RET_OVERFLOW -3
#define ISOTP_RET_WRONG_SN -4
#define ISOTP_RET_NO_DATA -5
#define ISOTP_RET_TIMEOUT -6
#define ISOTP_RET_LENGTH -7
#define ISOTP_RET_NOSPACE -8

/* return logic true if 'a' is after 'b' */
#define IsoTpTimeAfter(a, b) ((int32_t)((int32_t)(b) - (int32_t)(a)) < 0)

/*  invalid bs */
#define ISOTP_INVALID_BS 0xFFFF

/* Define the maximum amount of characters allowed in an error message. This fixes code which would otherwise break on Microsoft's dumb platform. */
#define ISOTP_MAX_ERROR_MSG_SIZE 128

/* ISOTP sender status */
typedef enum {
    ISOTP_SEND_STATUS_IDLE,
    ISOTP_SEND_STATUS_INPROGRESS,
    ISOTP_SEND_STATUS_ERROR,
} IsoTpSendStatusTypes;

/* ISOTP receiver status */
typedef enum {
    ISOTP_RECEIVE_STATUS_IDLE,
    ISOTP_RECEIVE_STATUS_INPROGRESS,
    ISOTP_RECEIVE_STATUS_FULL,
} IsoTpReceiveStatusTypes;

/* can fram defination */
#if defined(ISOTP_BYTE_ORDER_LITTLE_ENDIAN)
typedef struct {
    uint8_t reserve_1 : 4;
    uint8_t type      : 4;
    uint8_t reserve_2[ISO_TP_MAX_CAN_FRAME_SIZE - 1];
} IsoTpPciType;

typedef struct {
    uint8_t SF_DL : 4;
    uint8_t type  : 4;
    uint8_t data[ISO_TP_MAX_CAN_FRAME_SIZE - 1];
} IsoTpSingleFrame;

typedef struct {
    uint8_t set_to_zero : 4;
    uint8_t type        : 4;
    uint8_t SF_DL;
    uint8_t data[ISO_TP_MAX_CAN_FRAME_SIZE - 2];
} IsoTpSingleFrameEscape;

typedef struct {
    uint8_t FF_DL_high : 4;
    uint8_t type       : 4;
    uint8_t FF_DL_low;
    uint8_t data[ISO_TP_MAX_CAN_FRAME_SIZE - 2];
} IsoTpFirstFrameShort;

ISOTP_PACKED_STRUCT({
    uint8_t  set_to_zero_high : 4;
    uint8_t  type             : 4;
    uint8_t  set_to_zero_low;
    uint32_t FF_DL;
    uint8_t  data[ISO_TP_MAX_CAN_FRAME_SIZE - 6];
} IsoTpFirstFrameLong);

typedef struct {
    uint8_t SN   : 4;
    uint8_t type : 4;
    uint8_t data[ISO_TP_MAX_CAN_FRAME_SIZE - 1];
} IsoTpConsecutiveFrame;

typedef struct {
    uint8_t FS   : 4;
    uint8_t type : 4;
    uint8_t BS;
    uint8_t STmin;
    uint8_t reserve[ISO_TP_MAX_CAN_FRAME_SIZE - 3];
} IsoTpFlowControl;

#else

typedef struct {
    uint8_t type      : 4;
    uint8_t reserve_1 : 4;
    uint8_t reserve_2[ISO_TP_MAX_CAN_FRAME_SIZE - 1];
} IsoTpPciType;

/*
 * single frame
 * +-------------------------+-----+
 * | byte #0                 | ... |
 * +-------------------------+-----+
 * | nibble #0   | nibble #1 | ... |
 * +-------------+-----------+ ... +
 * | PCIType = 0 | SF_DL     | ... |
 * +-------------+-----------+-----+
 */
typedef struct {
    uint8_t type  : 4;
    uint8_t SF_DL : 4;
    uint8_t data[ISO_TP_MAX_CAN_FRAME_SIZE - 1];
} IsoTpSingleFrame;

/*
 * single frame using the SF_DL escape sequence (CAN FD only, CAN_DL > 8)
 * +-------------------------+-----------------------+-----+
 * | byte #0                 | byte #1               | ... |
 * +-------------------------+-----------+-----------+-----+
 * | nibble #0   | nibble #1 | nibble #2 | nibble #3 | ... |
 * +-------------+-----------+-----------+-----------+-----+
 * | PCIType = 0 | unused=0  | SF_DL                 | ... |
 * +-------------+-----------+-----------------------+-----+
 */
typedef struct {
    uint8_t type        : 4;
    uint8_t set_to_zero : 4;
    uint8_t SF_DL;
    uint8_t data[ISO_TP_MAX_CAN_FRAME_SIZE - 2];
} IsoTpSingleFrameEscape;

/*
 * first frame short
 * +-------------------------+-----------------------+-----+
 * | byte #0                 | byte #1               | ... |
 * +-------------------------+-----------+-----------+-----+
 * | nibble #0   | nibble #1 | nibble #2 | nibble #3 | ... |
 * +-------------+-----------+-----------+-----------+-----+
 * | PCIType = 1 | FF_DL                             | ... |
 * +-------------+-----------+-----------------------+-----+
 */
typedef struct {
    uint8_t type       : 4;
    uint8_t FF_DL_high : 4;
    uint8_t FF_DL_low;
    uint8_t data[ISO_TP_MAX_CAN_FRAME_SIZE - 2];
} IsoTpFirstFrameShort;

/*
 * first frame long
 * +-------------------------+-----------------------+---------+---------+---------+---------+
 * | byte #0                 | byte #1               | byte #2 | byte #3 | byte #4 | byte #5 |
 * +-------------------------+-----------+-----------+---------+---------+---------+---------+
 * | nibble #0   | nibble #1 | nibble #2 | nibble #3 | ...                                   |
 * +-------------+-----------+-----------+-----------+---------------------------------------+
 * | PCIType = 1 | unused=0  | escape sequence = 0   | FF_DL                                 |
 * +-------------+-----------+-----------------------+---------------------------------------+
 */
ISOTP_PACKED_STRUCT({
    uint8_t  type             : 4;
    uint8_t  set_to_zero_high : 4;
    uint8_t  set_to_zero_low;
    uint32_t FF_DL;
    uint8_t  data[ISO_TP_MAX_CAN_FRAME_SIZE - 6];
} IsoTpFirstFrameLong);

/*
 * consecutive frame
 * +-------------------------+-----+
 * | byte #0                 | ... |
 * +-------------------------+-----+
 * | nibble #0   | nibble #1 | ... |
 * +-------------+-----------+ ... +
 * | PCIType = 0 | SN        | ... |
 * +-------------+-----------+-----+
 */
typedef struct {
    uint8_t type : 4;
    uint8_t SN   : 4;
    uint8_t data[ISO_TP_MAX_CAN_FRAME_SIZE - 1];
} IsoTpConsecutiveFrame;

/*
 * flow control frame
 * +-------------------------+-----------------------+-----------------------+-----+
 * | byte #0                 | byte #1               | byte #2               | ... |
 * +-------------------------+-----------+-----------+-----------+-----------+-----+
 * | nibble #0   | nibble #1 | nibble #2 | nibble #3 | nibble #4 | nibble #5 | ... |
 * +-------------+-----------+-----------+-----------+-----------+-----------+-----+
 * | PCIType = 1 | FS        | BS                    | STmin                 | ... |
 * +-------------+-----------+-----------------------+-----------------------+-----+
 */
typedef struct {
    uint8_t type : 4;
    uint8_t FS   : 4;
    uint8_t BS;
    uint8_t STmin;
    uint8_t reserve[ISO_TP_MAX_CAN_FRAME_SIZE - 3];
} IsoTpFlowControl;

#endif

typedef struct {
        uint8_t ptr[ISO_TP_MAX_CAN_FRAME_SIZE];
} IsoTpDataArray;

typedef struct {
    union {
        IsoTpPciType           common;
        IsoTpSingleFrame       single_frame;
        IsoTpSingleFrameEscape single_frame_escape;
        IsoTpFirstFrameShort   first_frame_short;
        IsoTpFirstFrameLong    first_frame_long;
        IsoTpConsecutiveFrame  consecutive_frame;
        IsoTpFlowControl       flow_control;
        IsoTpDataArray         data_array;
    } as;
} IsoTpCanMessage;

/**************************************************************
 * protocol specific defines
 *************************************************************/

#ifdef ISO_TP_TRANSMIT_COMPLETE_CALLBACK
/* Private: Function pointer type for transmission done callback
 * Called when any transmission (single-frame or multi-frame) is completed successfully
 */
typedef void (*isotp_tx_done_cb)(void* link, uint32_t tx_size, void* user_arg);
#endif

#ifdef ISO_TP_RECEIVE_COMPLETE_CALLBACK
/* Private: Function pointer type for receive done callback
 * Called when a complete message (single-frame or multi-frame) has been received successfully
 */
typedef void (*isotp_rx_done_cb)(void* link, const uint8_t* data, uint32_t size, void* user_arg);
#endif

/* Private: Protocol Control Information (PCI) types, for identifying each frame of an ISO-TP message.
 */
typedef enum {
    ISOTP_PCI_TYPE_SINGLE             = 0x0,
    ISOTP_PCI_TYPE_FIRST_FRAME        = 0x1,
    TSOTP_PCI_TYPE_CONSECUTIVE_FRAME  = 0x2,
    ISOTP_PCI_TYPE_FLOW_CONTROL_FRAME = 0x3,

    ISOTP_PCI_TYPE_CONSECUTIVE_FRAME  = 0x2, // Typo fix; but keep broken value for backwards-compat.
} IsoTpProtocolControlInformation;

/* Private: Protocol Control Information (PCI) flow control identifiers.
 */
typedef enum { PCI_FLOW_STATUS_CONTINUE = 0x0, PCI_FLOW_STATUS_WAIT = 0x1, PCI_FLOW_STATUS_OVERFLOW = 0x2 } IsoTpFlowStatus;

/* Private: network layer resault code.
 */
#define ISOTP_PROTOCOL_RESULT_OK 0
#define ISOTP_PROTOCOL_RESULT_TIMEOUT_A -1
#define ISOTP_PROTOCOL_RESULT_TIMEOUT_BS -2
#define ISOTP_PROTOCOL_RESULT_TIMEOUT_CR -3
#define ISOTP_PROTOCOL_RESULT_WRONG_SN -4
#define ISOTP_PROTOCOL_RESULT_INVALID_FS -5
#define ISOTP_PROTOCOL_RESULT_UNEXP_PDU -6
#define ISOTP_PROTOCOL_RESULT_WFT_OVRN -7
#define ISOTP_PROTOCOL_RESULT_BUFFER_OVFLW -8
#define ISOTP_PROTOCOL_RESULT_ERROR -9

#endif // ISOTPC_USER_DEFINITIONS_H
