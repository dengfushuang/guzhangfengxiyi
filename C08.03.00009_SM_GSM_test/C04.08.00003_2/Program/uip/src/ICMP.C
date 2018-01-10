#include <string.h>
#include "uip_arp.h"
#include "uip.h"

#define   ARP_REQUEST 1
#define   ARP_REPLY   2

#define   ARP_HWTYPE_ETH 1
#define   BUF   ((struct arp_hdr *)&ICMP_buf[0])
#define   IPBUF ((struct ethip_hdr *)&ICMP_buf[0])
#define   ICMPBUF   ((struct uip_icmpip_hdr *)&ICMP_buf[UIP_LLH_LEN])

u8_t    ICMP_buf[74];
u16_t   icmp_seqno[4];
u16_t   icmp_recseqno[4];

extern  struct arp_entry arp_table[UIP_ARPTAB_SIZE];
extern  u16_t chksum(u16_t sum, const u8_t *data, u16_t len);

//ICMP����IP����У�����
u16_t ICMP_ipchksum(void)
{
    u16_t sum;
    sum = chksum(0, &ICMP_buf[UIP_LLH_LEN], UIP_IPH_LEN);
    return (sum == 0) ? 0xffff : htons(sum);
}
//ICMP����У�����
u16_t uip_icmpchksum(void)
{
    u16_t sum;
    sum = chksum(0, &ICMP_buf[UIP_IPH_LEN + UIP_LLH_LEN], 40);
    return (sum == 0) ? 0xffff : htons(sum);
}

/*****************************************************************************
**�� �� ��: ICMP_set
**��������: ICMP ���ݰ����� 
**�������: u8_t num  
**�������: ��
**�� �� ֵ: �ɹ����ͷ��� 1
*****************************************************************************/
u8_t ICMP_set(u8_t num)
{
    u8_t  i;
    u16_t ipaddr[2];
    struct arp_entry *tabptr;
    /* Else, we use the destination IP address. */
    uip_ipaddr_copy(ipaddr, uip_pingaddr[num]);

    for(i = 0; i < UIP_ARPTAB_SIZE; ++i)
    {
        tabptr = &arp_table[i];
        if(uip_ipaddr_cmp(ipaddr, tabptr->ipaddr))
        {
            break;
        }
    }
    if(i == UIP_ARPTAB_SIZE)
    {
        /* The destination address was not in our ARP table, so we
        overwrite the IP packet with an ARP request. */
        memset(BUF->ethhdr.dest.addr, 0xff, 6);
        memset(BUF->dhwaddr.addr, 0x00, 6);
        memcpy(BUF->ethhdr.src.addr, uip_ethaddr.addr, 6);
        memcpy(BUF->shwaddr.addr, uip_ethaddr.addr, 6);

        uip_ipaddr_copy(BUF->dipaddr, uip_pingaddr[num]);  //!!!!!!!!!!!!!!!!
        uip_ipaddr_copy(BUF->sipaddr, uip_hostaddr);
        BUF->opcode = HTONS(ARP_REQUEST); /* ARP request. */
        BUF->hwtype = HTONS(ARP_HWTYPE_ETH);
        BUF->protocol = HTONS(UIP_ETHTYPE_IP);
        BUF->hwlen = 6;
        BUF->protolen = 4;
        BUF->ethhdr.type = HTONS(UIP_ETHTYPE_ARP);

        //uip_appdata = &uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN];

        uip_len = sizeof(struct arp_hdr);
        return 0;
    }

    //--------------��̫���ײ�----------------
    memcpy(BUF->ethhdr.dest.addr, tabptr->ethaddr.addr, 6);
    memcpy(BUF->ethhdr.src.addr, uip_ethaddr.addr, 6);
    BUF->ethhdr.type = HTONS(UIP_ETHTYPE_IP);
    //--------------IP�ײ�----------------
    ICMPBUF->vhl=0x45;
    ICMPBUF->tos=0;
    ICMPBUF->len[0]=0x00;
    ICMPBUF->len[1]=0x3C;
    ++ipid;
    ICMPBUF->ipid[0] = ipid >> 8;
    ICMPBUF->ipid[1] = ipid & 0xff;
    ICMPBUF->ipoffset[0] = ICMPBUF->ipoffset[1] = 0; //�ֶ�ƫ����
    ICMPBUF->ttl=64;       //����ʱ��
    ICMPBUF->proto=1;      //�ϲ�Э��
    uip_ipaddr_copy(ICMPBUF->destipaddr, uip_pingaddr[num]);
    uip_ipaddr_copy(ICMPBUF->srcipaddr, uip_hostaddr);
    ICMPBUF->ipchksum = 0;
    ICMPBUF->ipchksum = ~(ICMP_ipchksum());
    //--------------ICMP�ײ�----------------
    ICMPBUF->type=8;      //����
    ICMPBUF->icode=0;
    ICMPBUF->id= 0x0002;
    ICMPBUF->seqno = icmp_seqno[num];     //���к�  //!!!!!!!!!!!!!!!!!!!!!
    icmp_seqno[num]++;

    i=0;
    while(i++<7) ICMP_buf[41+i]='6';
    ICMP_buf[48]=icmp_recseqno[num]/256;
    ICMP_buf[49]=icmp_recseqno[num]%256;

    // while(i++<26) ICMP_buf[41+i]=i+'A';
    // i=0;
    // while(i++<6)  ICMP_buf[68+i]=i+'A';
    ICMPBUF->icmpchksum=0;
    ICMPBUF->icmpchksum = ~( uip_icmpchksum( ) );

    uip_len = 74;
    return 1;
}
