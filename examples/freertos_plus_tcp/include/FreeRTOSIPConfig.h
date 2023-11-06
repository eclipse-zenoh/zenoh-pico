//
// Copyright (c) 2023 Fictionlab sp. z o.o.
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   Błażej Sowa, <blazej@fictionlab.pl>

#ifndef FREERTOS_IP_CONFIG_H
#define FREERTOS_IP_CONFIG_H

// Driver specific
#define ipconfigBYTE_ORDER pdFREERTOS_LITTLE_ENDIAN
#define ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES 1
#define ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM 0
#define ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM 1
#define ipconfigZERO_COPY_RX_DRIVER 1
#define ipconfigZERO_COPY_TX_DRIVER 1
#define ipconfigUSE_LINKED_RX_MESSAGES 1
#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS 60

// Enable/Disable features
#define ipconfigUSE_IPv4 1
#define ipconfigUSE_IPv6 1
#define ipconfigUSE_DHCP 1
#define ipconfigUSE_DHCPv6 0
#define ipconfigUSE_RA 0
#define ipconfigUSE_DNS 1
#define ipconfigUSE_TCP 1
#define ipconfigUSE_ARP_REMOVE_ENTRY 0
#define ipconfigUSE_ARP_REVERSED_LOOKUP 0
#define ipconfigSUPPORT_SELECT_FUNCTION 0
#define ipconfigSUPPORT_OUTGOING_PINGS 0
#define ipconfigUSE_NETWORK_EVENT_HOOK 1
#define ipconfigUSE_DHCP_HOOK 0

// DHCP
#define ipconfigMAXIMUM_DISCOVER_TX_PERIOD (pdMS_TO_TICKS(1000U))
#define ipconfigDHCP_REGISTER_HOSTNAME 1

#define ipconfigIP_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define ipconfigIP_TASK_STACK_SIZE_WORDS (configMINIMAL_STACK_SIZE * 5)

// Set ipconfigBUFFER_PADDING on 64-bit platforms
#if INTPTR_MAX == INT64_MAX
#define ipconfigBUFFER_PADDING 14U
#endif

#endif /* FREERTOS_IP_CONFIG_H */
