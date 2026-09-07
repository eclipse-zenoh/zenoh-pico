////////////////////////////////////////////////////////////////////////
//                  ___ ___  ___ _____ ___      ___                   //
//                 |_ _/ __|/ _ \_   _| _ \___ / __|                  //
//                  | |\__ \ (_) || | |  _/___| (__                   //
//                 |___|___/\___/ |_| |_|      \___|                  //
//                                                                    //
////////////////////////////////////////////////////////////////////////

#ifndef ISOTPC_USER_H
#define ISOTPC_USER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief user implemented, print debug message */
void isotp_user_debug(const char* message, ...);

/**
 * @brief user implemented, send can message. should return ISOTP_RET_OK when success.
 *
 * @param arbitration_id The CAN ID the frame has to be sent with.
 * @param data The frame data.
 * @param size The amount of bytes to send. This never exceeds 8 bytes unless the library is
 *             built for CAN FD (see ISO_TP_MAX_CAN_FRAME_SIZE), in which case it may be any
 *             valid CAN frame length of up to ISO_TP_MAX_CAN_FRAME_SIZE bytes. Sizes of more
 *             than 8 bytes always have to be transmitted as a CAN FD frame.
 * @param flags Only present if ISO_TP_USER_SEND_CAN_FLAGS is enabled: a combination of the
 *              ISOTP_CAN_FRAME_FLAG_* values describing the frame format to use. Frames sent
 *              by a link transmitting CAN FD frames (a TX_DL of more than 8 bytes) carry
 *              ISOTP_CAN_FRAME_FLAG_FD, irrespective of their length.
 * @param arg Only present if ISO_TP_USER_SEND_CAN_ARG is enabled: the value of the link's
 *            user_send_can_arg member, e.g. the CAN interface to send the frame on.
 *
 * @return may return ISOTP_RET_NOSPACE if the CAN transfer should be retried later
 * or ISOTP_RET_ERROR if transmission couldn't be completed
 */
int isotp_user_send_can(const uint32_t arbitration_id, const uint8_t* data, const uint8_t size
#ifdef ISO_TP_USER_SEND_CAN_FLAGS
                        , const uint8_t flags
#endif
#ifdef ISO_TP_USER_SEND_CAN_ARG
                        , void* arg
#endif
);

/**
 * @brief user implemented, gets the amount of time passed since the last call in microseconds
 */
uint32_t isotp_user_get_us(void);

#ifdef __cplusplus
}
#endif

#endif // ISOTPC_USER_H
