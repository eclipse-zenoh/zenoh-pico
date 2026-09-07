////////////////////////////////////////////////////////////////////////
//                  ___ ___  ___ _____ ___      ___                   //
//                 |_ _/ __|/ _ \_   _| _ \___ / __|                  //
//                  | |\__ \ (_) || | |  _/___| (__                   //
//                 |___|___/\___/ |_| |_|      \___|                  //
//                                                                    //
//                      ___ ___  _  _ ___ ___ ___                     //
//                     / __/ _ \| \| | __|_ _/ __|                    //
//                    | (_| (_) | .` | _| | | (_ |                    //
//                     \___\___/|_|\_|_| |___\___|                    //
//                                                                    //
////////////////////////////////////////////////////////////////////////

#ifndef ISOTPC_CONFIG_H
#define ISOTPC_CONFIG_H

/* The maximum amount of data bytes a single CAN frame may carry (CAN_DL).
 * Classical CAN is limited to 8 bytes; CAN FD additionally allows frames of
 * 12, 16, 20, 24, 32, 48 and 64 bytes.
 *
 * Set this to one of the CAN FD lengths to enable CAN FD support. This
 * increases the size of the internal frame buffers accordingly, so leave it at
 * 8 on platforms without CAN FD.
 */
#ifndef ISO_TP_MAX_CAN_FRAME_SIZE
    #define ISO_TP_MAX_CAN_FRAME_SIZE 8
#endif

/* The CAN_DL (TX_DL) used for frames transmitted by a freshly initialised link.
 * This may be reduced per link at runtime using isotp_set_tx_dl(), e.g. when a
 * peer only supports Classical CAN frame lengths.
 */
#ifndef ISO_TP_DEFAULT_TX_DL
    #define ISO_TP_DEFAULT_TX_DL ISO_TP_MAX_CAN_FRAME_SIZE
#endif

/* Max number of messages the receiver can receive at one time, this value
 * is affected by can driver queue length
 */
#ifndef ISO_TP_DEFAULT_BLOCK_SIZE
    #define ISO_TP_DEFAULT_BLOCK_SIZE 8
#endif

/* The STmin parameter value specifies the minimum time gap allowed between
 * the transmission of consecutive frame network protocol data units
 */
#ifndef ISO_TP_DEFAULT_ST_MIN_US
    #define ISO_TP_DEFAULT_ST_MIN_US 0
#endif

/* This parameter indicate how many FC N_PDU WTs can be transmitted by the
 * receiver in a row.
 */
#ifndef ISO_TP_MAX_WFT_NUMBER
    #define ISO_TP_MAX_WFT_NUMBER 1
#endif

/* Private: The default timeout to use when waiting for a response during a
 * multi-frame send or receive.
 */
#ifndef ISO_TP_DEFAULT_RESPONSE_TIMEOUT_US
    #define ISO_TP_DEFAULT_RESPONSE_TIMEOUT_US 100000
#endif

/* Private: Determines if by default, padding is added to ISO-TP message frames.
 */
// #define ISO_TP_FRAME_PADDING

/* Private: Omit the two formatted error messages, which are the library's only
 * use of snprintf(). Define this on a target whose libc has no snprintf, or
 * where the 128-byte ISOTP_MAX_ERROR_MSG_SIZE stack buffer is unwelcome. The
 * errors are still reported through isotp_user_debug(), without the values.
 *
 * Measured on Cortex-M4, -Os -DNDEBUG: .text 2235 -> 2091 bytes, and the
 * largest stack frame 160 -> 32 bytes. With this and NDEBUG the object needs
 * nothing from libc but memcpy and memset.
 */
// #define ISO_TP_NO_FORMATTED_ERRORS


/* Private: Value to use when padding frames if enabled by ISO_TP_FRAME_PADDING
 */
#ifndef ISO_TP_FRAME_PADDING_VALUE
    #define ISO_TP_FRAME_PADDING_VALUE 0xAA
#endif

/* Private: Determines if by default, an additional argument is present in the
 * definition of isotp_user_send_can.
 */
// zenoh-pico: ENABLED. The send hook otherwise has no context, and
// the port needs to know which socket to put the frame on. Passing it through
// `user_send_can_arg` is explicit; the alternative -- deducing the link from
// the arbitration id -- happens to work today only because every frame a link
// emits, flow control included, carries its own tx_id, and that is too subtle
// a thing to rely on.
//
// isotp_config.h is upstream's designated configuration point, so editing it
// is the intended way to do this. isotp.c and isotp.h remain verbatim. The
// define MUST be seen by every translation unit that includes isotp.h -- which
// is exactly why it lives here rather than in a build flag.
#define ISO_TP_USER_SEND_CAN_ARG

/* Private: Determines if a frame flags argument is present in the definition of
 * isotp_user_send_can, telling the CAN driver whether a frame has to be
 * transmitted as a CAN FD frame. Enable this if the driver cannot derive the
 * frame format from the frame length on its own.
 */
// #define ISO_TP_USER_SEND_CAN_FLAGS

/* Private: Determines if CAN FD frames are flagged for transmission using the
 * data phase bit rate (bit rate switch). Only used if ISO_TP_USER_SEND_CAN_FLAGS
 * is enabled.
 */
// #define ISO_TP_CAN_FD_USE_BRS

/* Enable support for transmission complete callback */
// #define ISO_TP_TRANSMIT_COMPLETE_CALLBACK

/* Enable support for receive complete callback */
// #define ISO_TP_RECEIVE_COMPLETE_CALLBACK

/* Enable support for receiving messages larger than the receive buffer in
 * application-consumable chunks.
 */
// #define ISO_TP_ENABLE_STREAMING

#endif // ISOTPC_CONFIG_H
