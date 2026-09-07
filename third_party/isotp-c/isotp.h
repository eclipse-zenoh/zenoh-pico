////////////////////////////////////////////////////////////////////////
//                  ___ ___  ___ _____ ___      ___                   //
//                 |_ _/ __|/ _ \_   _| _ \___ / __|                  //
//                  | |\__ \ (_) || | |  _/___| (__                   //
//                 |___|___/\___/ |_| |_|      \___|                  //
//                                                                    //
////////////////////////////////////////////////////////////////////////
                                                                  
#ifndef ISOTPC_H
#define ISOTPC_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
    #include <stdint.h>

extern "C" {
#endif

#include "isotp_config.h"
#include "isotp_defines.h"
#include "isotp_user.h"

/**
 * @brief Struct containing the data for linking an application to a CAN instance.
 * The data stored in this struct is used internally and may be used by software programs
 * using this library.
 */
typedef struct IsoTpLink {
    /* sender paramters */
    uint32_t            send_arbitration_id; /* used to reply consecutive frame */
    uint8_t             tx_dl;               /* CAN_DL of transmitted frames; 8 for Classical CAN, up to 64 for CAN FD */

    /* message buffer */
    uint8_t*            send_buffer;
    uint32_t            send_buf_size;
    uint32_t            send_size;
    uint32_t            send_offset;

    /* multi-frame flags */
    uint8_t             send_sn;
    uint32_t            send_bs_remain; /* Remaining block size */
    uint32_t            send_st_min_us; /* Separation Time between consecutive frames */
    uint8_t             send_wtf_count; /* Maximum number of FC.Wait frame transmissions  */
    uint32_t            send_timer_st;  /* Last time send consecutive frame */
    uint32_t            send_timer_bs;  /* Time until reception of the next FlowControl N_PDU
                                           start at sending FF, CF, receive FC
                                           end at receive FC */
    int32_t             send_protocol_result;
    uint8_t             send_status;

    /* receiver paramters */
    uint32_t            receive_arbitration_id;
    uint8_t             rx_dl;                 /* CAN_DL of the multi-frame message currently being received */
    
    /* message buffer */
    uint8_t*            receive_buffer;
    uint32_t            receive_buf_size;
    uint32_t            receive_size;
    uint32_t            receive_offset;

    /* multi-frame control */
    uint8_t             receive_sn;
    uint8_t             receive_bs_count; /* Maximum number of FC.Wait frame transmissions  */
    uint32_t            receive_timer_cr; /* Time until transmission of the next ConsecutiveFrame N_PDU
                                    start at sending FC, receive CF
                                    end at receive FC */
    int                 receive_protocol_result;
    uint8_t             receive_status;

#ifdef ISO_TP_ENABLE_STREAMING
    uint32_t            receive_stream_size; /* Bytes currently available in receive_buffer */
    uint8_t             receive_streaming;   /* The current message is larger than receive_buffer */
    uint8_t             receive_stream_carry_size;
    uint8_t             receive_stream_carry[ISO_TP_MAX_CAN_FRAME_SIZE - 1]; /* Remainder of a frame crossing a chunk boundary */
#endif

#if defined(ISO_TP_USER_SEND_CAN_ARG)
    void*               user_send_can_arg;
#endif

#ifdef ISO_TP_TRANSMIT_COMPLETE_CALLBACK
    isotp_tx_done_cb    tx_done_cb;     /* User callback for transmission complete */
    void*               tx_done_cb_arg; /* User argument for callback */
#endif

#ifdef ISO_TP_RECEIVE_COMPLETE_CALLBACK
    isotp_rx_done_cb    rx_done_cb;     /* User callback for receive complete */
    void*               rx_done_cb_arg; /* User argument for callback */
#endif

} IsoTpLink;

/**
 * @brief Initialises the ISO-TP library.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 * @param sendid The ID used to send data to other CAN nodes.
 * @param sendbuf A pointer to an area in memory which can be used as a buffer for data to be sent.
 * @param sendbufsize The size of the buffer area.
 * @param recvbuf A pointer to an area in memory which can be used as a buffer for data to be received.
 * @param recvbufsize The size of the buffer area.
 *
 * @remarks The link is initialised with a transmit frame length (TX_DL) of
 * @code ISO_TP_DEFAULT_TX_DL @endcode. Use @link isotp_set_tx_dl @endlink to change it.
 */
void isotp_init_link(IsoTpLink* link, uint32_t sendid, uint8_t* sendbuf, uint32_t sendbufsize, uint8_t* recvbuf, uint32_t recvbufsize);

/**
 * @brief Sets the CAN frame data length (TX_DL) used for transmitted frames.
 *
 * Classical CAN links must use 8; CAN FD links may additionally use 12, 16, 20, 24, 32, 48 or 64.
 * Values larger than @code ISO_TP_MAX_CAN_FRAME_SIZE @endcode are rejected, as the library is not
 * compiled with buffers large enough to hold such frames.
 *
 * Received frames are always handled irrespective of this value; it only affects transmission.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 * @param tx_dl The CAN frame data length to use for transmitted frames.
 *
 * @return Possible return values:
 *  - @code ISOTP_RET_OK @endcode
 *  - @code ISOTP_RET_ERROR @endcode if the link is null or the length is not a valid CAN_DL
 *  - @code ISOTP_RET_INPROGRESS @endcode if a multi-frame transmission is currently in progress
 */
int isotp_set_tx_dl(IsoTpLink* link, uint8_t tx_dl);

/**
 * @brief Gets the CAN frame data length (TX_DL) used for transmitted frames.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 *
 * @return The configured TX_DL, or 0 if the link is null.
 */
uint8_t isotp_get_tx_dl(const IsoTpLink* link);

/**
 * @brief Destroys the ISO-TP link and releases associated resources.
 *
 * @param link The @code IsoTpLink @endcode instance to destroy.
 */
void isotp_destroy_link(IsoTpLink* link);

/**
 * @brief Polling function; call this function periodically to handle timeouts, send consecutive frames, etc.
 *
 * @param link The @code IsoTpLink @endcode instance used.
 */
void isotp_poll(IsoTpLink* link);

/**
 * @brief Handles incoming CAN messages.
 * Determines whether an incoming message is a valid ISO-TP frame or not and handles it accordingly.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 * @param data The data received via CAN.
 * @param len The length of the data received. Frames longer than
 *            @code ISO_TP_MAX_CAN_FRAME_SIZE @endcode are ignored.
 */
void isotp_on_can_message(IsoTpLink* link, const uint8_t* data, uint8_t len);

/**
 * @brief Sends ISO-TP frames via CAN, using the ID set in the initialising function.
 *
 * Single-frame messages will be sent immediately when calling this function.
 * Multi-frame messages will be sent consecutively when calling isotp_poll.
 *
 * Payloads which do not fit into a single frame (more than 7 bytes with a TX_DL of 8,
 * more than TX_DL - 2 bytes on CAN FD links) are segmented. Messages larger than 4095
 * bytes are sent using the escaped first frame format defined by ISO 15765-2:2016.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 * @param payload The payload to be sent.
 * @param size The size of the payload to be sent.
 *
 * @return Possible return values:
 *  - @code ISOTP_RET_OVERFLOW @endcode
 *  - @code ISOTP_RET_INPROGRESS @endcode
 *  - @code ISOTP_RET_OK @endcode
 *  - The return value of the user shim function isotp_user_send_can().
 */
int isotp_send(IsoTpLink* link, const uint8_t payload[], uint32_t size);

/**
 * @brief See @link isotp_send @endlink, with the exception that this function is used only for functional addressing.
 */
int isotp_send_with_id(IsoTpLink* link, uint32_t id, const uint8_t payload[], uint32_t size);

/**
 * @brief Receives and parses the received data and copies the parsed data in to the internal buffer.
 * @param link The @link IsoTpLink @endlink instance used to transceive data.
 * @param payload A pointer to an area in memory where the raw data is copied from.
 * @param payload_size The size of the received (raw) CAN data.
 * @param out_size A reference to a variable which will contain the size of the actual (parsed) data.
 *
 * @return Possible return values:
 *      - @link ISOTP_RET_OK @endlink
 *      - @link ISOTP_RET_NO_DATA @endlink
 */
int isotp_receive(IsoTpLink* link, uint8_t* payload, const uint32_t payload_size, uint32_t* out_size);

#ifdef ISO_TP_ENABLE_STREAMING
/**
 * @brief Receives the next available chunk of an ISO-TP message.
 *
 * When an incoming multi-frame message is larger than the link's receive
 * buffer, reception is paused using ISO-TP flow control whenever that buffer
 * is full. Calling this function consumes the buffered chunk and allows the
 * sender to continue. The function may also be used for messages which fit in
 * the receive buffer.
 *
 * @param link The @link IsoTpLink @endlink instance used to receive data.
 * @param payload Destination for the available chunk.
 * @param payload_size Size of the destination buffer.
 * @param out_size Receives the number of bytes copied to @p payload.
 * @param is_complete Set to true when the returned chunk ends the message.
 *
 * @return @link ISOTP_RET_OK @endlink, @link ISOTP_RET_NO_DATA @endlink,
 *         @link ISOTP_RET_NOSPACE @endlink, or @link ISOTP_RET_ERROR @endlink.
 */
int isotp_receive_streaming(IsoTpLink* link, uint8_t* payload, const uint32_t payload_size, uint32_t* out_size, bool* is_complete);
#endif

#ifdef ISO_TP_TRANSMIT_COMPLETE_CALLBACK
/**
 * @brief Sets the callback function for transmission complete notification.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 * @param cb The callback function to be called when transmission is complete.
 * @param arg A pointer that will be passed to the callback function.
 */
void isotp_set_tx_done_cb(IsoTpLink* link, isotp_tx_done_cb cb, void* arg);
#endif

#ifdef ISO_TP_RECEIVE_COMPLETE_CALLBACK
/**
 * @brief Sets the callback function for receive complete notification.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 * @param cb The callback function to be called when a message is received.
 * @param arg A pointer that will be passed to the callback function.
 */
void isotp_set_rx_done_cb(IsoTpLink* link, isotp_rx_done_cb cb, void* arg);
#endif

#ifdef __cplusplus
}
#endif

#endif // ISOTPC_H
