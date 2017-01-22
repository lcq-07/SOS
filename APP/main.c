/**
 * @file    main.c
 * @brief   main.c
 * @author  Andy.Luo
 * @date    2016-12-20 10:33
 * @par    
 * Change History :
 * <Date> <Version> <Author> <Description>
 */

#include "sos.h"

#define REG32_W(adr, value) *(unsigned int *)adr=value
#define REG32_R(adr, value) value = *(unsigned int *)adr

#define VTOR_LEN 128
#define VTOR_MSP (0)
#define VTOR_RST (1)
#define VTOR_NMI (2)
#define VTOR_HRD (3)
#define VTOR_MEM (4)
#define VTOR_BUS (5)
#define VTOR_USE (6)
#if 0
#define VTOR_RSV (7)
#define VTOR_RSV (8)
#define VTOR_RSV (9)
#define VTOR_RSV (10)
#endif
#define VTOR_SVC (11)
#define VTOR_DBG (12)
#define VTOR_RSV (13)
#define VTOR_PSV (14)
#define VTOR_STK (15)
#define IRQ_BASE (16)
#define NVIC_VTOR 0xE000ED08

unsigned int rst_vtor;
void *new_vtor[VTOR_LEN] __attribute__((at(0x20005000 - VTOR_LEN*sizeof(void *))));

int irq_cnt = 0;
int svc_cnt = 0;
int psv_cnt = 0;

void sys_fault(void)
{
	int i;

	for(i=1000; i>0; i--);
}

void init_vector(void)
{
	int i;
	int v;

	REG32_R(NVIC_VTOR, rst_vtor);

	for(i=0; i<10; i++) {
		v = *(unsigned int *)rst_vtor;
		new_vtor[i] = (void *)v;
		rst_vtor += 4;
	}
	for(; i<VTOR_LEN; i++) {
		new_vtor[i] = (void *)i;
	}
	
#if 0
	new_vtor[VTOR_MSP] = (void *)0x20000400;
	new_vtor[VTOR_RST] = &sys_fault;
        new_vtor[VTOR_NMI] = &sys_fault;
        new_vtor[VTOR_HRD] = &sys_fault;
        new_vtor[VTOR_MEM] = &sys_fault;
        new_vtor[VTOR_BUS] = &sys_fault;
        new_vtor[VTOR_USE] = &sys_fault;
        new_vtor[VTOR_RSV] = &sys_fault;
        new_vtor[VTOR_RSV] = &sys_fault;
        new_vtor[VTOR_RSV] = &sys_fault;
        new_vtor[VTOR_RSV] = &sys_fault;
#endif
        new_vtor[VTOR_SVC] = &sys_svc;
        new_vtor[VTOR_DBG] = &sys_fault;
        new_vtor[VTOR_RSV] = &sys_fault;
        new_vtor[VTOR_PSV] = &sys_psv;
        new_vtor[VTOR_STK] = &sys_stk;

	REG32_W(NVIC_VTOR, (unsigned int)new_vtor);


}

unsigned int x_stack[128/sizeof(unsigned int)];
int x_cnt = 0;
void x_task(void)
{
	int ret;
	int tsk_id;

	while(1) {
		for(ret=0x90000; ret>0; ret--);
		tsk_id = os_get_self_tskid();
		os_delete_tsk(tsk_id);
	}
}

unsigned int a_stack[128/sizeof(unsigned int)];
int a_cnt = 0;
void a_task(void)
{
	int ret;
	int sum;
	int tsk_id = 0;

	sum = 0;
	while(1) {
		sum += 1;
		if (tsk_id <= 0) {
			tsk_id = os_create_tsk(x_task, (void *)x_stack, sizeof(x_stack), 0);
		}
		for(ret=0xA0000; ret>0; ret--);
		if (os_get_tsk_status(tsk_id) == TSK_DELET) {
			tsk_id = 0;
		}
		ret = call_svc_n(0xa, 0xb, 0xc, 0xd);
		for(ret=0xA0000; ret>0; ret--);
	}
}
unsigned int b_stack[128/sizeof(unsigned int)];
int b_cnt = 0;
void b_task(void)
{
	int ret;
	int sum;
	int a[8];

	sum = 0;
	for(ret=0; ret<8; ret++) {
		a[ret] = 0;
	}
	while(1) {
		sum += 1;
		a[sum&0x7] += 1;
		ret = call_svc_n(0xaa, 0xbb, 0xcc, 0xdd);
		for(ret=0xA0000; ret>0; ret--);
	}
}
unsigned int c_stack[128/sizeof(unsigned int)];
int c_cnt = 0;
void c_task(void)
{
	int ret;
	int sum;

	sum = 0;
	while(1) {
		sum += 1;
		for(ret=0xA0000; ret>0; ret--);
	}
}
unsigned int d_stack[128/sizeof(unsigned int)];
int d_cnt = 0;
void d_task(void)
{
	int ret;
	int sum;
	
	sum = 0;
	while(1) {
		sum += 1;
		for(ret=0xA0000; ret>0; ret--);
	}
}




int main(int argc, char **argv)
{
	setup_sp();
	init_vector();
	os_init();
	os_create_tsk(a_task, (void *)a_stack, sizeof(a_stack), 1);
	os_create_tsk(b_task, (void *)b_stack, sizeof(b_stack), 1);
	os_create_tsk(c_task, (void *)c_stack, sizeof(c_stack), 1);
	os_create_tsk(d_task, (void *)d_stack, sizeof(d_stack), 1);
	os_start();

	/* never reach to this place */
	return -1;
}
