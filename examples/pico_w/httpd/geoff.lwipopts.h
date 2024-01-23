/*
 * Copyright (c) 2022 Geoff Simmons <geoff@simmons.de>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * See LICENSE
 */

/*
 * Sample lwipopts.h for applications that use picow-http
 *
 * Set MAX_CONCURRENT_CX_HINT to the maximum number of concurrent client
 * connections that the HTTP server should be able to support. Other
 * values for lwIP are then set to fulfill that requirement.
 *
 * See also:
 * - "Configuring lwipopts.h" in the picow-http Wiki
 *   https://gitlab.com/slimhazard/picow_http/-/wikis/Configuring-lwipopts.h
 * - README in the current directory
 * - lwIP documentation for lwipopts.h
 *   https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html
 */

#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

#ifndef NO_SYS
#define NO_SYS                      1
#endif

/*
 * The default for MAX_CONCURRENT_CX_HINT is a conservative value, so that
 * the default memory footprint of picow-http is small.
 *
 * This is a common value among most browsers for the maximum number of
 * client connections to a single server IP address. It should usually be
 * sufficient if a single client application connects to the server.
 *
 * Increase this value if more concurrent client connections are required.
 */
#ifndef MAX_CONCURRENT_CX_HINT
#define MAX_CONCURRENT_CX_HINT      6
#endif

#define NUM_SERVER_HINT             1

#define LWIP_SOCKET                 0
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1
#else
// MEM_LIBC_MALLOC is incompatible with non polling versions
#define MEM_LIBC_MALLOC             0
#endif
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    (MAX_CONCURRENT_CX_HINT * TCP_MSS)
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define TCP_MSS                     1460
#if !(LWIP_ALTCP_TLS)
# define TCP_WND                    (MAX_CONCURRENT_CX_HINT * TCP_MSS)
# elif  (MAX_CONCURRENT_CX_HINT * TCP_MSS) > (16 * 1024)
# define TCP_WND                    (MAX_CONCURRENT_CX_HINT * TCP_MSS)
# else
# define TCP_WND                    (16 * 1024)
#endif
#define TCP_SND_BUF                 (MAX_CONCURRENT_CX_HINT * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define TCP_LISTEN_BACKLOG          1

#if !(LWIP_ALTCP_TLS)
# define MEMP_NUM_TCP_SEG           32
# else
# define MEMP_NUM_TCP_SEG           TCP_SND_QUEUELEN
#endif

#define MEMP_NUM_TCP_PCB            (MAX_CONCURRENT_CX_HINT)
#define MEMP_NUM_TCP_PCB_LISTEN     (NUM_SERVER_HINT)

#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETCONN                0
#define LINK_STATS                  0
#define LWIP_CHKSUM_ALGORITHM       3
#define LWIP_DHCP                   1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define DHCP_DOES_ARP_CHECK         0
#define LWIP_DHCP_DOES_ACD_CHECK    0

#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#endif

#define MEM_STATS                   0
#define SYS_STATS                   0
#define TCP_STATS                   0
#define IP_STATS                    0
#define MEMP_STATS                  0

#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define ALTCP_MBEDTLS_DEBUG         LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */
