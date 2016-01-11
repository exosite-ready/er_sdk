
/* UIP "helper"
 * Implements the UIP application
 * These has to be first currently, becuase on PC platform
 * UIP_HTONS *HAS TO* be redefined by the uip version
 * This is pretty UGLY
 **/
#include "uip_port.h"

#include <sdk_config.h>
#include <lib/type.h>
#include <lib/tcp_buffer.h>
#include <lib/sf_malloc.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/pclb.h>
#include <lib/utils.h>

#include <system_utils.h>
#include <rtc_if.h>

#include "uip_adapter_conf.h"
#include "uip.h"
#include "uip_arp.h"

#include "uip-split.h"
#include "dhcpc.h"
#include "resolv.h"
#include <uip-udp-packet.h>
#include "../../net_port.h"
#include "platform_uip.h"
#include "uip_net.h"

static struct link_layer_if *ll_if;
/* UIP send buffer */
extern void *uip_sappdata;

/* Global "configured" flag */
volatile u8 uip_configured;

/* *****************************************************************************
* Platform independenet UIP "main loop" implementation
**/

/* Timers */
static u32 arp_timer;

/* Macro for accessing the Ethernet header information in the buffer. */
#define BUF                     ((struct uip_eth_hdr *)&uip_buf[0])

/* UIP Timers (in ms) */
#define UIP_PERIODIC_TIMER_MS   10
#define UIP_ARP_TIMER_MS        10000

#define IP_TCP_HEADER_LENGTH 40
#define TOTAL_HEADER_LENGTH (IP_TCP_HEADER_LENGTH+UIP_LLH_LEN)

extern uint16_t uip_slen;
/*---------------------------------------------------------------------------*/
void uip_udp_packet_send(struct uip_udp_conn *c, const void *data, int len)
{
    system_enter_critical_section();
#if UIP_UDP
    if (data != NULL) {
        uip_udp_conn = c;
        uip_slen = len;
        memcpy(&uip_buf[UIP_LLH_LEN + UIP_IPUDPH_LEN], data,
               len > UIP_BUFSIZE - UIP_LLH_LEN - UIP_IPUDPH_LEN ?
               UIP_BUFSIZE - UIP_LLH_LEN - UIP_IPUDPH_LEN : len);
        uip_process(UIP_UDP_SEND_CONN);

#if UIP_CONF_IPV6_MULTICAST
        /* Let the multicast engine process the datagram before we send it */
        if (uip_is_addr_mcast_routable(&uip_udp_conn->ripaddr))
            UIP_MCAST6.out();
#endif                          /* UIP_IPV6_MULTICAST */

    }
    uip_arp_out();
    uip_slen = 0;
    system_leave_critical_section();
    ll_if->send_packet(uip_buf, uip_len);
#endif                          /* UIP_UDP */
}

/*---------------------------------------------------------------------------*/
void uip_udp_packet_sendto(struct uip_udp_conn *c, const void *data, int len,
                            const uip_ipaddr_t *toaddr, uint16_t toport)
{
    uip_ipaddr_t curaddr;
    uint16_t curport;

    if (toaddr != NULL) {
        /* Save current IP addr/port. */
        uip_ipaddr_copy(&curaddr, &c->ripaddr);
        curport = c->rport;

        /* Load new IP addr/port */
        uip_ipaddr_copy(&c->ripaddr, toaddr);
        c->rport = toport;

        uip_udp_packet_send(c, data, len);

        /* Restore old IP addr/port */
        uip_ipaddr_copy(&c->ripaddr, &curaddr);
        c->rport = curport;
    }
}

static void device_driver_send(void)
{
    ll_if->send_packet(uip_buf, uip_len);
}

/* This gets called on both Ethernet RX interrupts and timer requests, */
/* but it's called only from the Ethernet interrupt handler */
static unsigned int start;      /* 0 */
void uip_mainloop(void *dummy)
{
    sys_time_t temp;
    u32 packet_len;

    /* Increment uIP timers */
    if (ll_if == NULL)
        return;

    rtc_periodic(NULL);
    temp = system_get_diff_time(start);
    start += temp;
    arp_timer += temp;

    /* Check for an RX packet and read it */
    packet_len = ll_if->get_packet_nb(uip_buf, sizeof(uip_buf));
    if (packet_len > 0) {
        /* Set uip_len for uIP stack usage. */
        uip_len = (unsigned short)packet_len;

        /* Process incoming IP packets here. */
        if (BUF->type == uip_htons(UIP_ETHTYPE_IP)) {
            uip_arp_ipin();
            uip_input();

            /* If the above function invocation resulted in data that
             *  should be sent out on the network, the global variable
             *  uip_len is set to a value > 0.
             **/
            if (uip_len > 0) {
                uip_arp_out();
                device_driver_send();
            }
        }
        /* Process incoming ARP packets here. */
        else if (BUF->type == uip_htons(UIP_ETHTYPE_ARP)) {
            uip_arp_arpin();

            /* If the above function invocation resulted in data that
             *  should be sent out on the network, the global variable
             *  uip_len is set to a value > 0.
             **/
            if (uip_len > 0)
                device_driver_send();
        }
    }
    for (temp = 0; temp < UIP_CONNS; temp++) {
        uip_periodic(temp);

        /* If the above function invocation resulted in data that should
         * be sent out on the network, the global variable uip_len is set
         *  to a value > 0.
         */
        if (uip_len > 0) {
            uip_arp_out();
            device_driver_send();
        }
    }

#if UIP_UDP
    for (temp = 0; temp < UIP_UDP_CONNS; temp++) {
        uip_udp_periodic(temp);

        /* If the above function invocation resulted in data that should
         *  be sent out on the network, the global variable uip_len is set
         *  to a value > 0.
         **/
        if (uip_len > 0) {
            uip_arp_out();
            device_driver_send();
        }
    }
#endif	/* UIP_UDP */

    /* Process ARP Timer here. */
    if (arp_timer >= UIP_ARP_TIMER_MS) {
        arp_timer = 0;
        uip_arp_timer();
    }
}

/* ***************************************************************************** */
/* DHCP callback */

#ifdef BUILD_DHCPC
static void uip_conf_static(void);

void dhcpc_configured(const struct dhcpc_state *s)
{
    if (s->ipaddr[0] != 0) {
        uip_sethostaddr(s->ipaddr);
        uip_setnetmask(s->netmask);
        uip_setdraddr(s->default_router);
        resolv_conf((u16_t *) s->dnsaddr);
        uip_configured = 1;
    } else
        uip_conf_static();
}

#endif

/* ***************************************************************************** */
/* DNS callback */

#ifdef BUILD_DNS
static volatile int resolv_req_done;
static uip_net_ip resolv_ip;

void resolv_found(char *name, u16_t *ipaddr)
{
    (void)name;
    if (!ipaddr)
        resolv_ip.ipaddr = 0;
    else {
        resolv_ip.ipwords[0] = ipaddr[0];
        resolv_ip.ipwords[1] = ipaddr[1];
    }
    resolv_req_done = 1;
}

#endif

/* ***************************************************************************** */
/* Console over Ethernet support */

#ifdef BUILD_CON_TCP

/* TELNET specific data */
#define TELNET_IAC_CHAR        255
#define TELNET_IAC_3B_FIRST    251
#define TELNET_IAC_3B_LAST     254
#define TELNET_SB_CHAR         250
#define TELNET_SE_CHAR         240
#define TELNET_EOF             236

/* The telnet socket number */
static int uip_telnet_socket = -1;

/* Utility function for TELNET: parse input buffer, skipping over
 * TELNET specific sequences
 * Returns the length of the buffer after processing
 **/
static void uip_telnet_handle_input(volatile struct uip_state *s)
{
    u8 *dptr = (u8 *) uip_appdata;
    char *orig = (char *)s->ptr;
    int skip;
    uip_net_size maxsize = s->len;

    /* Traverse the input buffer, skipping over TELNET sequences */
    while ((dptr < (u8 *) uip_appdata + uip_datalen()) && (s->ptr - orig < s->len)) {
        if (*dptr != TELNET_IAC_CHAR)   /* regular char, copy it to buffer */
            *s->ptr++ = *dptr++;
        else {
            /* Control sequence: 2 or 3 bytes? */
            if ((dptr[1] >= TELNET_IAC_3B_FIRST) && (dptr[1] <= TELNET_IAC_3B_LAST))
                skip = 3;
            else {
                /* Check EOF indication */
                if (dptr[1] == TELNET_EOF)
                    *s->ptr++ = STD_CTRLZ_CODE;
                skip = 2;
            }
            dptr += skip;
        }
    }
    if (s->ptr > orig) {
        s->res = ERR_SUCCESS;
        s->len = maxsize - (s->ptr - orig);
        uip_stop();
        s->state = UIP_STATE_IDLE;
    }
}

/* Utility function for TELNET: prepend all '\n' with '\r' in buffer
 * Returns actual len
 * It is assumed that the buffer is "sufficiently smaller" than the UIP
 * buffer (which is true for the default configuration: 128 bytes buffer
 * in Newlib for stdin/stdout, more than 1024 bytes UIP buffer)
 **/
static uip_net_size uip_telnet_prep_send(const char *src, uip_net_size size)
{
    uip_net_size actsize = size, i;
    char *dest = (char *)uip_sappdata;

    for (i = 0; i < size; i++) {
        if (*src == '\n') {
            *dest++ = '\r';
            actsize++;
        }
        *dest++ = *src++;
    }
    return actsize;
}

#endif                          /* #ifdef BUILD_CON_TCP */

/* ***************************************************************************** */
/* UIP application (used to implement the TCP/IP services) */

/* Special handling for "accept" */
static volatile u8 uip_accept_request;
static volatile int uip_accept_sock;
static volatile uip_net_ip uip_accept_remote;

void uip_appcall(void)
{
    volatile struct uip_state *s;
    uip_net_size temp;
    int sockno;

    /* If uIP is not yet configured (DHCP response not received), do
     *  nothing
     **/
    if (!uip_configured)
        return;
    s = (struct uip_state *)&(uip_conn->appstate);
    /* Need to find the actual socket location, since UIP doesn't provide
     *  this ...
     **/
    for (temp = 0; temp < UIP_CONNS; temp++)
        if (uip_conns + temp == uip_conn)
            break;
    sockno = (int)temp;

    if (uip_connected()) {
        if (uip_accept_request) {
            uip_accept_sock = sockno;
            uip_accept_remote.ipwords[0] = uip_conn->ripaddr[0];
            uip_accept_remote.ipwords[1] = uip_conn->ripaddr[1];
            uip_accept_request = 0;
        }
        s->connected = 1;
        s->res = ERR_SUCCESS;
        uip_stop();
        return;
    }
    if (s->state == UIP_STATE_RECV) {
        uip_restart();
        s->state = UIP_STATE_IDLE;
    }
    if (uip_aborted() || uip_timedout() || uip_closed()) {
        /* Signal this error */
        s->res = uip_aborted() ? ERR_NET_ABORTED : (uip_timedout() ? ERR_NET_TIMEDOUT : ERR_NET_CLOSED);
        return;
    }
    if (s->state == UIP_STATE_CLOSE) {
        uip_close();
        s->state = UIP_STATE_IDLE;
    }
    /* Handle data send */
    if ((uip_acked() || uip_rexmit() || uip_poll())) {
        /* TODO why was it it here: Special translation for TELNET:
         *  prepend all '\n' with '\r'
         **/
        /* Temporarily remove put back when we need that */

        if (uip_acked()) {
            uip_net_size minlen = UMIN(s->outbuf_len, uip_mss());

            s->outbuf_len -= minlen;
            s->outptr += minlen;

            if (s->outptr == 0)
                s->res = ERR_SUCCESS;
        }
        if (s->outbuf_len > 0)  /* need to (re)transmit? */
            uip_send(s->outptr, UMIN(s->outbuf_len, uip_mss()));
    }
    /* Handle data receive */
    if (uip_newdata()) {
        if (uip_datalen()) {
            temp = uip_datalen();

            s->readto = UIP_NET_NO_LASTCHAR;
            s->with_buffer = 0;

#ifdef TCPBUF
            tcp_buffer_push(s->tcp_inbuf, uip_appdata, temp);
            int tcp_free_size = tcp_buffer_get_free(s->tcp_inbuf);

            if (tcp_free_size < UIP_RECEIVE_WINDOW) {
                trac_log(DEBUG_NET, ("UIP stop"));
                uip_stop();
            }
#else
            sf_memcpy((char *)s->inbuf, (const char *)uip_appdata, temp);
            s->inbuf_len = temp;
            s->inptr = s->inbuf;

            uip_stop();
#endif
            s->res = ERR_SUCCESS;
            s->state = UIP_STATE_IDLE;
        }
    }
}

static void uip_conf_static(void)
{
    uip_ipaddr_t ipaddr;

    uip_ipaddr(ipaddr, UIP_CONF_IPADDR0, UIP_CONF_IPADDR1, UIP_CONF_IPADDR2, UIP_CONF_IPADDR3);
    uip_sethostaddr(ipaddr);
    uip_ipaddr(ipaddr, UIP_CONF_NETMASK0, UIP_CONF_NETMASK1, UIP_CONF_NETMASK2, UIP_CONF_NETMASK3);
    uip_setnetmask(ipaddr);
    uip_ipaddr(ipaddr, UIP_CONF_DEFGW0, UIP_CONF_DEFGW1, UIP_CONF_DEFGW2, UIP_CONF_DEFGW3);
    uip_setdraddr(ipaddr);
    uip_ipaddr(ipaddr, UIP_CONF_DNS0, UIP_CONF_DNS1, UIP_CONF_DNS2, UIP_CONF_DNS3);
    resolv_conf(ipaddr);
    uip_configured = 1;
}

/*Store the MAC address here so that the applications can query this from here */
static struct uip_eth_addr g_eth_addr;

void uip_get_eth_addr(struct uip_eth_addr *paddr)
{
    *paddr = g_eth_addr;
}

/* Init application */
void net_dev_init(void)
{
    int c;
    static struct mac_addr maddr;
    static struct uip_eth_addr *paddr;

    /* Initialize the low level interface */
    platform_ll_device_init(&ll_if);
    ASSERT(ll_if);

    ll_if->get_mac(&maddr);
    paddr = (struct uip_eth_addr *)&maddr;

    pclb_register(UIP_PERIODIC_TIMER_MS, uip_mainloop, NULL, NULL);

    /* Set hardware address */
    uip_setethaddr((*paddr));
    /* Save the hardware address */
    g_eth_addr = *paddr;

    /* Initialize the uIP TCP/IP stack. */
    uip_init(system_get_time());
    uip_arp_init();

    /* Initalize the socket specific part the uIP specific part is
     *  initalized in uip_init
     **/
    for (c = 0; c < UIP_CONNS; ++c) {
        uip_conns[c].appstate.inuse = 0;
        uip_conns[c].appstate.inbuf = 0;
        uip_conns[c].appstate.tcp_inbuf = 0;
        uip_conns[c].appstate.outbuf = 0;
    }

#ifdef BUILD_DHCPC
    dhcpc_init(paddr->addr, sizeof(*paddr));
    dhcpc_request();
#else
    uip_conf_static();
#endif

    resolv_init();

#ifdef BUILD_CON_TCP
    uip_listen(UIP_HTONS(UIP_NET_TELNET_PORT));
#endif
}

/* ***************************************************************************** */
/* UIP UDP application (used for the DHCP client and the DNS resolver) */
void uip_udp_appcall(void)
{
    resolv_appcall();
    dhcpc_appcall();
    udp_socket_receive_appcall();
}

/* **************************************************************************** */
/* TCP/IP services (from uip_net.h)*/

#define UIP_IS_SOCK_OK(sock) (uip_configured && sock >= 0 && sock < UIP_CONNS)

static void prep_socket_state(volatile struct uip_state *pstate,
                                   void *buf, uip_net_size len, s16 readto, u8 res, int with_buffer, u8 state)
{
    system_enter_critical_section();

    pstate->ptr = (char *)buf;
    pstate->len = len;
    pstate->res = res;
    pstate->readto = readto;
    pstate->with_buffer = with_buffer;
    pstate->state = state;

    system_leave_critical_section();
}

int uip_net_socket(int type, int non_blocking)
{
    int i;
    struct uip_conn *pconn;

    /* [TODO] add UDP support at some point. */
    if (type == UIP_NET_SOCK_DGRAM)
        return -1;

    system_enter_critical_section();
    /* Iterate through the list of connections, looking for a free one */
    for (i = 0; i < UIP_CONNS; i++) {
        pconn = uip_conns + i;
        if (pconn->tcpstateflags == UIP_CLOSED && !pconn->appstate.inuse) {
            /* Found a free connection, reserve it for later use */
            uip_conn_reserve(i);

#ifdef TCPBUF
            if (pconn->appstate.tcp_inbuf)
                tcp_buffer_delete(pconn->appstate.tcp_inbuf);
#else
            if (pconn->appstate.inbuf)
                sf_free(pconn->appstate.inbuf);
#endif
            if (pconn->appstate.outbuf)
                sf_free(pconn->appstate.outbuf);

            pconn->appstate.inuse = 1;
#ifdef TCPBUF
            tcp_buffer_new(&pconn->appstate.tcp_inbuf, 5000);
#else
            pconn->appstate.inbuf = sf_malloc(UIP_CONF_BUFFER_SIZE);
            pconn->appstate.inbuf_max_len = UIP_CONF_BUFFER_SIZE;
            pconn->appstate.inbuf_len = 0;
#endif
            pconn->appstate.outbuf = sf_malloc(UIP_CONF_BUFFER_SIZE);
            pconn->appstate.outbuf_max_len = UIP_CONF_BUFFER_SIZE;
            pconn->appstate.outbuf_len = 0;
            pconn->appstate.non_blocking = non_blocking;
            pconn->appstate.connect_started = 0;
            pconn->appstate.connected = 0;
            pconn->appstate.res = ERR_SUCCESS;

            break;
        }
    }

    if (i == UIP_CONNS)
        error_log(DEBUG_NET, ("UIP tcp socket error: No free connections\n"), -1);

    system_leave_critical_section();
    return i == UIP_CONNS ? -1 : i;
}

/* Send data */
uip_net_size uip_net_send(int s, const void *buf, uip_net_size len)
{
    int ret;
    volatile struct uip_state *pstate = (volatile struct uip_state *)&(uip_conns[s].appstate);

    if (len == 0)
        return 0;

    system_enter_critical_section();

    if (!UIP_IS_SOCK_OK(s) || !uip_conn_active(s)) {
        error_log(DEBUG_NET, ("UIP tcp send error %d, %d\n", s, uip_conns[s].tcpstateflags), -1);
        system_leave_critical_section();
        return -1;
    }
    if (pstate->outbuf_len == 0) {
        int size_to_send;

        if (len >= pstate->outbuf_max_len)
            size_to_send = pstate->outbuf_max_len;
        else
            size_to_send = len;

        memcpy(pstate->outbuf, buf, size_to_send);
        pstate->outptr = pstate->outbuf;
        pstate->outbuf_len = size_to_send;
        prep_socket_state(pstate, (void *)pstate->outbuf, size_to_send,
                UIP_NET_NO_LASTCHAR, ERR_SUCCESS, 0, UIP_STATE_SEND);
        /* system_eth_force_interrupt(); */
        ret = size_to_send;
    } else {
        if (pstate->res == ERR_SUCCESS)
            pstate->res = ERR_WOULD_BLOCK;
        ret = -1;
    }

    system_leave_critical_section();
    return ret;
}

/* Internal "read" function */
static uip_net_size net_recv_internal(int s, void *buf, uip_net_size maxsize,
                                            s16 readto, unsigned timer_id, timer_data_type to_us, int with_buffer)
{
    volatile struct uip_state *pstate = (volatile struct uip_state *)&(uip_conns[s].appstate);
    int to_copy;
    int ret;
    (void)readto;
    (void)timer_id;
    (void)to_us;
    (void)with_buffer;

    if (maxsize == 0)
        return 0;

    system_enter_critical_section();
    if (!UIP_IS_SOCK_OK(s) || !uip_conn_active(s)) {
        system_leave_critical_section();
        return -1;
    }
#ifdef TCPBUF
    int tcp_buffer_used_size = tcp_buffer_get_used(pstate->tcp_inbuf);
#else
    int tcp_buffer_used_size = pstate->inbuf_len;
#endif
    if (tcp_buffer_used_size == 0) {
        pstate->state = UIP_STATE_RECV;
        if (pstate->res == ERR_SUCCESS)
            pstate->res = ERR_WOULD_BLOCK;
        ret = -1;
    } else {
        if (maxsize <= tcp_buffer_used_size)
            to_copy = maxsize;
        else
            to_copy = tcp_buffer_used_size;

#ifdef TCPBUF
        tcp_buffer_pop(pstate->tcp_inbuf, buf, to_copy);
#else
        memcpy(buf, pstate->inptr, to_copy);
        pstate->inbuf_len -= to_copy;
        pstate->inptr += to_copy;
#endif
        ret = to_copy;
    }

    system_leave_critical_section();
    return ret;
}

/*Receive data in buf, upto "maxsize" bytes, or upto the 'readto' character if it's not -1*/
uip_net_size uip_net_recv(int s, void *buf, uip_net_size maxsize,
                            s16 readto, unsigned timer_id, timer_data_type to_us)
{
    return net_recv_internal(s, buf, maxsize, readto, timer_id, to_us, 0);
}

/*No lua support at this layer*/
#if 0
/* Same thing, but with a Lua buffer as argument */
uip_net_size uip_net_recvbuf(int s, luaL_Buffer *buf,
                               uip_net_size maxsize, s16 readto, unsigned timer_id, timer_data_type to_us)
{
    return net_recv_internal(s, buf, maxsize, readto, timer_id, to_us, 1);
}

#endif

/* Return the socket associated with the "telnet" application(or - 1 if it does // not exist)
 * .The socket only exists if a client connected to the board.
 **/
int uip_net_get_telnet_socket(void)
{
    int res = -1;

#ifdef BUILD_CON_TCP
    if (uip_telnet_socket != -1)
        if (uip_conn_active(uip_telnet_socket))
            res = uip_telnet_socket;
#endif
    return res;
}

/*Close socket*/
int uip_net_close(int s)
{
    volatile struct uip_state *pstate = (volatile struct uip_state *)&(uip_conns[s].appstate);

    if (!UIP_IS_SOCK_OK(s)) {
        error_log(DEBUG_NET, ("UIP tcp socket close: invalid socket\n"), -1);
        return -1;
    }
    system_enter_critical_section();
    pstate->inuse = 0;
    if (!uip_conn_active(s)) {
        if (uip_conns[s].tcpstateflags == UIP_RESERVED)
            error_log(DEBUG_NET, ("UIP tcp socket close: ERROR: inactive socket returning with reserved\n"), -1);

        uip_conns[s].tcpstateflags = UIP_CLOSED;
        system_leave_critical_section();
        return -1;
    }
    /* system_eth_force_interrupt(); */
    pstate->state = UIP_STATE_CLOSE;
    system_leave_critical_section();
    return pstate->res == ERR_SUCCESS ? 0 : -1;
}

/* Get last error on specific socket */
int uip_net_get_last_err(int s)
{
    volatile struct uip_state *pstate = (volatile struct uip_state *)&(uip_conns[s].appstate);
    int result;

    if (!UIP_IS_SOCK_OK(s))
        return -1;

    system_enter_critical_section();
    result = pstate->res;
    system_leave_critical_section();
    return result;
}

/* Accept a connection on the given port, return its socket id(and the IP of the remote host by side effect) */
int uip_accept(u16 port, unsigned timer_id, timer_data_type to_us, uip_net_ip *pfrom)
{
    return -1;
    /* Currently not supported */
#if 0
        timer_data_type tmrstart = 0;

    if (!uip_configured)
        return -1;
#ifdef BUILD_CON_TCP
    if (port == UIP_NET_TELNET_PORT)
        return -1;
#endif
    system_enter_critical_section();
    uip_unlisten(uip_htons(port));
    uip_listen(uip_htons(port));
    system_leave_critical_section();
    uip_accept_sock = -1;
    uip_accept_request = 1;
    if (to_us > 0)
        tmrstart = platform_timer_start(timer_id);
    while (1) {
        if (uip_accept_request == 0)
            break;
        if (to_us > 0 && platform_timer_get_diff_crt(timer_id, tmrstart) >= to_us) {
            uip_accept_request = 0;
            break;
        }
    }
    *pfrom = uip_accept_remote;
    return uip_accept_sock;
#endif
}

/* Connect to a specified machine */
int uip_net_connect(int s, uip_net_ip addr, u16 port)
{
    volatile struct uip_state *pstate = (volatile struct uip_state *)&(uip_conns[s].appstate);
    uip_ipaddr_t ipaddr;

    if (!UIP_IS_SOCK_OK(s))
        return ERR_FAILURE;

    system_enter_critical_section();
    if (!pstate->connect_started) {
        /* The socket should have been reserved by a previous call to
         *  "uip_net_socket"
         */
        if (!uip_conn_is_reserved(s)) {
            system_leave_critical_section();
            return ERR_FAILURE;
        }
        /* Initiate the connect call */
        pstate->connect_started = 1;
        uip_ipaddr(ipaddr, addr.ipbytes[0], addr.ipbytes[1], addr.ipbytes[2], addr.ipbytes[3]);
        if (uip_connect_socket(s, &ipaddr, uip_htons(port)) == NULL) {
            system_leave_critical_section();
            return ERR_FAILURE;
        }
    }
    /* And wait for it to finish*/
            if (pstate->non_blocking) {
            if (pstate->connected) {
                pstate->connect_started = 0;
            } else {
                if (pstate->res == ERR_SUCCESS)
                    pstate->res = ERR_WOULD_BLOCK;

                system_leave_critical_section();
                return pstate->res;
            }
        } else {
            while (pstate->connected)
                ;
            pstate->connect_started = 0;
        }

    system_leave_critical_section();
    return pstate->res;
}

/* Hostname lookup (resolver) */
int uip_net_lookup(const char *hostname, uip_net_ip *res)
{
    res->ipaddr = 0;
    static int in_progress;

#ifdef BUILD_DNS
    u16_t *data;

    /* The resolving process is already running */
    if (in_progress) {
        if (resolv_req_done) {
            in_progress = 0;
            if (resolv_ip.ipaddr == 0) {
                error_log(DEBUG_NET, ("Resolving host name is FAILED\n"), ERR_FAILURE);
                return ERR_FAILURE;
            } else {
                info_log(DEBUG_NET, ("Resolving host name is done\n"));
                *res = resolv_ip;
                return ERR_SUCCESS;
            }
        }
    } else {
        data = resolv_lookup((char *)hostname);
        if (data != NULL) {     /* Name already saved locally */
            res->ipwords[0] = data[0];
            res->ipwords[1] = data[1];
            return ERR_SUCCESS;
        } else {         /* Name not saved locally, must make a new request */
            resolv_req_done = 0;
            in_progress = 1;
            resolv_query((char *)hostname);
            info_log(DEBUG_NET, ("Resolving host name...\n"));
        }
    }
#endif
    return ERR_WOULD_BLOCK;
}
