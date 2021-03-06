#include "mac.h"
#include "irq.h"
#include "type.h"
#include "screen.h"
#include "syscall.h"
#include "sched.h"
#include "test4.h"
#define DESC_SIZE 16
queue_t recv_block_queue;
desc_t *send_desc;
desc_t *receive_desc;
uint32_t cnt = 1; //record the time of iqr_mac
//uint32_t buffer[PSIZE] = {0x00040045, 0x00000100, 0x5d911120, 0x0101a8c0, 0xfb0000e0, 0xe914e914, 0x00000801,0x45000400, 0x00010000, 0x2011915d, 0xc0a80101, 0xe00000fb, 0x14e914e9, 0x01080000};
uint32_t buffer[PSIZE] = {0xffffffff, 0x5500ffff, 0xf77db57d, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0004e914, 0x0000, 0x005e0001, 0x2300fb00, 0x84b7f28b, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0801e914, 0x0000};

static void mac_send_desc_init(mac_t *mac)
{
    uint32_t TxBuff = TXBUFFADDR;
    bzero((uint8_t *)TxBuff, PNUM*BUFFSIZE);
    send_desc     = (uint32_t)SEND_DESC_PTR;
    uint32_t addr = (uint32_t)SEND_DESC_PTR;
    int i;
    for(i=0;i<PNUM-1;addr += DESC_SIZE,i++)
    {
        //printf("12 %d\n",i);
        ((desc_t *)addr)->tdes0 = 0;
        //printf("13 %d\n",i);
        ((desc_t *)addr)->tdes1 = 0 | (1 << 24) | (1 << 31) | (1<<30) | (1<<29) |(BUFFSIZE & 0x7ff);
        //printf("14 %d\n",i);
        ((desc_t *)addr)->tdes2 = TxBuff & 0x1fffffff;
        //printf("15 %d\n",i);
        ((desc_t *)addr)->tdes3 = (addr + DESC_SIZE)& 0x1fffffff;
        memcpy((uint8_t *)TxBuff, buffer, sizeof(buffer));
        TxBuff += BUFFSIZE;
    }

    ((desc_t *)addr)->tdes0 = 0;
    ((desc_t *)addr)->tdes1 = 0 | (1 << 25) | (0 << 31) |  (1<<30) | (1<<29) |(BUFFSIZE & 0x7ff);
    ((desc_t *)addr)->tdes2 = TxBuff& 0x1fffffff;
    ((desc_t *)addr)->tdes3 = SEND_DESC_PTR& 0x1fffffff;
    memcpy((uint8_t *)TxBuff, buffer, sizeof(buffer));

    mac->td = (uint32_t)send_desc;
    mac->td_phy = (uint32_t)send_desc & 0x1fffffff;
    mac->saddr = TXBUFFADDR;
    mac->saddr_phy = TXBUFFADDR & 0x1fffffff;

}

static void mac_recv_desc_init(mac_t *mac)
{
    uint32_t RxBuff = RXBUFFADDR;
    bzero((uint8_t *)RxBuff, PNUM*BUFFSIZE);
    receive_desc  = (uint32_t)RECV_DESC_PTR;
    uint32_t addr = (uint32_t)RECV_DESC_PTR;
    int i;
    for(i=0;i<PNUM-1;addr += DESC_SIZE,i++)
    {
        ((desc_t *)addr)->tdes0 = 0;
        ((desc_t *)addr)->tdes1 = 0 | (1 << 24) | (1 << 31) | (BUFFSIZE & 0x7ff);
        ((desc_t *)addr)->tdes2 = RxBuff & 0x1fffffff;
        ((desc_t *)addr)->tdes3 = (addr + DESC_SIZE)& 0x1fffffff;
        RxBuff += BUFFSIZE;
    }

    ((desc_t *)addr)->tdes0 = 0;
    ((desc_t *)addr)->tdes1 = 0 | (1 << 25) | (0 << 31) | (BUFFSIZE & 0x7ff);
    ((desc_t *)addr)->tdes2 = RxBuff & 0x1fffffff;
    ((desc_t *)addr)->tdes3 = RECV_DESC_PTR & 0x1fffffff;
    
    mac->rd = (uint32_t)receive_desc;
    mac->rd_phy = (uint32_t)receive_desc & 0x1fffffff;
    mac->daddr = RXBUFFADDR;
    mac->daddr_phy = RXBUFFADDR & 0x1fffffff;
}
static uint32_t printf_recv_buffer(uint32_t recv_buffer)
{
    int i, print_location = 4;
    sys_move_cursor(1, print_location + 1);
    uint32_t *data = recv_buffer;
    for(i = 0; i < 256; i++)
        printf("%x ", data[i]);
    printf("\n");
    return 1;
}
static void mii_dul_force(mac_t *mac)
{
    reg_write_32(mac->dma_addr, 0x80); //?s
                                       //   reg_write_32(mac->dma_addr, 0x400);
    uint32_t conf = 0xc800;            //0x0080cc00;

    // loopback, 100M
    reg_write_32(mac->mac_addr, reg_read_32(mac->mac_addr) | (conf) | (1 << 8));
    //enable recieve all
    reg_write_32(mac->mac_addr + 0x4, reg_read_32(mac->mac_addr + 0x4) | 0x80000001);
}

void dma_control_init(mac_t *mac, uint32_t init_value)
{
    reg_write_32(mac->dma_addr + DmaControl, init_value);
    return;
}

void mac_send_task()
{

    mac_t test_mac;
    uint32_t i;
    uint32_t print_location = 2;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum

    mac_send_desc_init(&test_mac);

    dma_control_init(&test_mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&test_mac);

    mii_dul_force(&test_mac);

    // register_irq_handler(LS1C_MAC_IRQ, mac_irq_handle);

    // irq_enable(LS1C_MAC_IRQ);
    sys_move_cursor(1, print_location);
    printf("> [MAC SEND TASK] start send package.               \n");

    uint32_t cnt = 0;
    i = 4;
    while (i > 0)
    {
        sys_net_send(test_mac.td, test_mac.td_phy);
        cnt += PNUM;
        sys_move_cursor(1, print_location);
        printf("> [MAC SEND TASK] totally send package %d !        \n", cnt);
        i--;
        sys_sleep(1);
    }
    sys_exit();
}
//mac_t test_mac2;
void mac_recv_task()
{

    uint32_t i;
    uint32_t ret;
    uint32_t print_location = 1;

    test_mac2.mac_addr = 0xbfe10000;
    test_mac2.dma_addr = 0xbfe11000;

    test_mac2.psize = PSIZE * 4; // 64bytes
    test_mac2.pnum = PNUM;       // pnum
    mac_recv_desc_init(&test_mac2);

    dma_control_init(&test_mac2, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&test_mac2);

    mii_dul_force(&test_mac2);

    queue_init(&recv_block_queue);
    sys_move_cursor(1, print_location);
    printf("[MAC RECV TASK] start recv:                    ");
    ret = sys_net_recv(test_mac2.rd, test_mac2.rd_phy, test_mac2.daddr);
    if (ret == 0)
    {
        sys_move_cursor(1, print_location);
        printf("[MAC RECV TASK]     net recv is ok!                          ");
    }
    else
    {
        sys_move_cursor(1, print_location);
        printf("[MAC RECV TASK]     net recv is fault!                       ");
    }

    ch_flag = 0;
    for (i = 0; i < PNUM; i++)
    {
        recv_flag[i] = 0;
    }

    uint32_t cnt = 0;
    uint32_t *Recv_desc;
    Recv_desc = (uint32_t *)(test_mac2.rd + (PNUM - 1) * 16);
    //printf("(test_mac2.rd 0x%x ,Recv_desc=0x%x,REDS0 0X%x\n", test_mac2.rd, Recv_desc, *(Recv_desc));
    sys_move_cursor(1, 2);
    printf("de1:%x",test_mac2);
    if (((*Recv_desc) & 0x80000000) == 0x80000000)
    {
        sys_move_cursor(1, print_location);
        printf("> [MAC RECV TASK] waiting receive package.\n");
        sys_wait_recv_package();
    }

    //init_screen();
    //cnt += PNUM;
    sys_move_cursor(1, print_location);
    printf("> [MAC RECV TASK]print recv  buffer        \n");
    uint32_t recv_buffer, snd_buffer;
    desc_t *recv = NULL;
    recv_buffer = test_mac2.daddr;
    snd_buffer = test_mac2.saddr;
    print_location = 3;

    uint32_t valid_num = 0;
    for (i = 0; i < PNUM; i++)
    {
        if (recv_flag[i] == 0)
        {
            sys_move_cursor(1, print_location);
            printf("> [MAC RECV TASK]still waiting receive %dth package.             \n", i);
            sys_wait_recv_package();
        }

        Recv_desc = (uint32_t *)(test_mac2.rd + i * 16);
        recv = (desc_t *)Recv_desc;

        sys_move_cursor(1, print_location);
        printf("\n%d recv buffer,r_desc( 0x%x) =0x%x:          \n", i, Recv_desc, *(Recv_desc));

        recv_buffer = recv->tdes2;
        valid_num += printf_recv_buffer((recv->tdes2 | 0xa0000000));
        sys_sleep(1);
        printf("\n");
    }

    print_location = 3;
    sys_move_cursor(1, print_location);
    printf("\nrecv valid %d packages!:\n                ", valid_num);

    sys_exit();
}

void mac_init_task()
{
    uint32_t print_location = 1;
    sys_move_cursor(1, print_location);
    printf("> [MAC INIT TASK] Waiting for MAC initialization .\n");
    sys_init_mac();
    print_location = 2;
    sys_move_cursor(1, print_location);
    printf("> [MAC INIT TASK] MAC initialization succeeded.           \n");
    sys_exit();
}
