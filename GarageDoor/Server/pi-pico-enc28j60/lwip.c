#include "lwip/inet.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/stats.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "enc28j60.h"

// BUILD COMMAND:
// "cd build; mingw32-make;  cp .\pico_spi_ethernet.uf2 G:\; cd .."

// TOTAL PROJECT COST
// Pico : $6
// Relay: $6
// enc28j60 : $3
// Wires : $1
// TOTAL: $16 NZD

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS 17
#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_TRIGGER     
#define PIN_LED 25

static struct netif server_netif;
struct netif *echo_netif;

// based on example from: https://www.nongnu.org/lwip/2_0_x/group__lwip__nosys.html
#define ETHERNET_MTU 1500

uint8_t mac[6] = {0xAA, 0x6F, 0x77, 0x47, 0x75, 0x8C};

static err_t netif_output(struct netif *netif, struct pbuf *p)
{
    LINK_STATS_INC(link.xmit);

    // lock_interrupts();
    // pbuf_copy_partial(p, mac_send_buffer, p->tot_len, 0);
    /* Start MAC transmit here */

    printf("enc28j60: Sending packet of len %d\n", p->len);
    enc28j60PacketSend(p->len, (uint8_t *)p->payload);
    // pbuf_free(p);

    // error sending
    if (enc28j60Read(ESTAT) & ESTAT_TXABRT)
    {
        // a seven-byte transmit status vector will be
        // written to the location pointed to by ETXND + 1,
        printf("ERR - transmit aborted\n");
    }

    if (enc28j60Read(EIR) & EIR_TXERIF)
    {
        printf("ERR - transmit interrupt flag set\n");
    }

    // unlock_interrupts();
    return ERR_OK;
}

static void netif_status_callback(struct netif *netif)
{
    printf("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
}

static err_t netif_initialize(struct netif *netif)
{
    netif->linkoutput = netif_output;
    netif->output = etharp_output;
    // netif->output_ip6 = ethip6_output;
    netif->mtu = ETHERNET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
    // MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 100000000);
    SMEMCPY(netif->hwaddr, mac, sizeof(netif->hwaddr));
    netif->hwaddr_len = sizeof(netif->hwaddr);
    return ERR_OK;
}

void triggerGarageDoor() {
    printf("Opening Garage Door...\n");
    gpio_put(PIN_TRIGGER, 1);
    gpio_put(PIN_LED, 1);
    sleep_ms(200);
    gpio_put(PIN_TRIGGER, 0);
    gpio_put(PIN_LED, 0);
    printf("Opened Garage Door!\n");
}

// UDP Server Code modified from https://www.programmersought.com/article/80673949068/

//Define the port number
#define UDP_SERVER_PORT    20001   /* define the UDP local connection port */

//Declare the receive data callback function, specified in the initialization function
void udp_echoserver_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

//UDP server initialization function
void udp_echoserver_init(void)
{
    struct udp_pcb *upcb;
    err_t err;

    /* Create a new UDP control block  */
    upcb = udp_new();  //Create a new UDP control block

    if (upcb)
    {
        /* Bind the upcb to the UDP_PORT port */
        /* Using IP_ADDR_ANY allow the upcb to be used by any local interface */
        err = udp_bind(upcb, IP_ADDR_ANY, UDP_SERVER_PORT);   //Bind local IP address and port

        if(err == ERR_OK)
        {
            /* Set a receive callback for the upcb */
            udp_recv(upcb, udp_echoserver_receive_callback, NULL);   //Register callback function for receiving data
        }
        else
        {
            udp_remove(upcb);
        }
    }
}

//The server receives the data callback function
void udp_echoserver_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    printf("UDP DATA: %s\n\n\n\n\n", p->payload);
    if (strstr(p->payload, "MESSAGE_HERE") != NULL) {
        triggerGarageDoor();
    }
    /* Tell the client that we have accepted it */
    udp_sendto(upcb, p,addr,port);  //Echo data
    /* Free the p buffer */
    pbuf_free(p);
}

void main(void)
{
    stdio_init_all();

    // data sheet up to 20 mhz
    spi_init(SPI_PORT, 1 * 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_TRIGGER, GPIO_FUNC_SIO);
    gpio_set_function(PIN_LED, GPIO_FUNC_SIO);

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    gpio_set_dir(PIN_TRIGGER, GPIO_OUT);
    gpio_put(PIN_TRIGGER, 0);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 0);
    // END PICO INIT
    
    for (int i = 5; i > 0; i--)
    {
        printf("Sleeping for %d seconds...\n", i);
        sleep_ms(1000);
    }
    
    ip_addr_t addr, mask, static_ip;
    IP4_ADDR(&static_ip, 192, 168, 1, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);
    IP4_ADDR(&addr, 192, 168, 1, 1);

    struct netif netif;
    lwip_init();
    // IP4_ADDR_ANY if using DHCP client
    netif_add(&netif, &static_ip, &mask, &addr, NULL, netif_initialize, netif_input);
    netif.name[0] = 'e';
    netif.name[1] = '0';
    // netif_create_ip6_linklocal_address(&netif, 1);
    // netif.ip6_autoconfig_enabled = 1;
    netif_set_status_callback(&netif, netif_status_callback);
    netif_set_default(&netif);
    netif_set_up(&netif);

    dhcp_inform(&netif);
    // dhcp_start(&netif);

    enc28j60Init(mac);
    uint8_t *eth_pkt = malloc(ETHERNET_MTU);
    struct pbuf *p = NULL;

    netif_set_link_up(&netif);

    udp_echoserver_init();

    while (1)
    {
        uint16_t packet_len = enc28j60PacketReceive(ETHERNET_MTU, (uint8_t *)eth_pkt);
        if (packet_len)
        {
            //printf("enc: Received packet of length = %d\n", packet_len);
            p = pbuf_alloc(PBUF_RAW, packet_len, PBUF_POOL);
            pbuf_take(p, eth_pkt, packet_len);
            free(eth_pkt);
            eth_pkt = malloc(ETHERNET_MTU);
        }

        if (packet_len && p != NULL)
        {
            LINK_STATS_INC(link.recv);

            if (netif.input(p, &netif) != ERR_OK)
            {
                pbuf_free(p);
            }
        }

        /* Cyclic lwIP timers check */
        sys_check_timeouts();
    }
}