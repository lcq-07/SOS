/**
 * @file    sos.c
 * @brief   sos.c
 * @author  Andy.Luo
 * @date    2016-12-22 09:26
 * @par    
 * Change History :
 * <Date> <Version> <Author> <Description>
 */

#include "sos.h"

#define ksp 0x20004000
#define tsp (ksp - 8*1024)
#define NVIC_INT_CTL 0xe000ed04
#define pendsv_set   0x10000000


struct sos_tcb *running_tcb;
struct sos_tcb *tsk_ready[PRIO_SUM];

__asm void setup_sp(void)
{
	LDR R0, =ksp
	LDR R1, =tsp
	MSR MSP, R0
	MSR PSP, R1
	BX LR
}

void svc_handler(unsigned int *svc_sp)
{
	unsigned int svc_no;
	unsigned int svc_r0;
	unsigned int svc_r1;
	unsigned int svc_r2;
	unsigned int svc_r3;

	svc_no = ((char *)svc_sp[6])[-2];
	svc_r0 = svc_sp[0];
	svc_r1 = svc_sp[1];
	svc_r2 = svc_sp[2];
	svc_r3 = svc_sp[3];
	/* 开始调用具体系统函数 */
	/* 返回SVC调用返回值svc_sp[0] */
	switch(svc_no) {
	case 0:
		svc_sp[0] = sys_stk_init((int)svc_r0);
		break;
	case 1:
		svc_sp[0] = sys_create_tsk((void *)svc_r0, (void *)svc_r1, (int)svc_r2, (char)svc_r3);
		break;
	case 2:
		svc_sp[0] = sys_delete_tsk((struct sos_tcb *)svc_r0);
		break;
	case 3:
		svc_sp[0] = sys_get_tsk_status((struct sos_tcb *)svc_r0);
		break;
	case 4:
		svc_sp[0] = sys_get_self_tskid();
		break;
	default:
		svc_r0 = svc_r0; /* make compiler happy */
		svc_r1 = svc_r1;
		svc_r2 = svc_r2;
		svc_r3 = svc_r3;
		svc_sp[0] = 0xFFFFFFFF; /* XXX return -1 */
		break;
	}
}
__asm void sys_svc(void)
{
	EXTERN svc_handler
#if 1
	TST LR, #4
	ITE EQ
	MRSEQ R0, MSP
	MRSNE R0, PSP
#else
	MRS R0, PSP
#endif
	B svc_handler
}

int sys_stk_init(int ms)
{
	unsigned int CALIB;
	unsigned int reload_cnt;

	ms = ms;
	CALIB = *(unsigned int *)0xE000E01C; /* stk校准数值，23:0 10ms计数值 */
	reload_cnt = (CALIB<<2)>>2; /* stk计数值 */
	if (reload_cnt == 0) {
		reload_cnt = 0x10000;
	}
	*(unsigned int *)0xE000E014 = reload_cnt; /* stk计数值 */
	if (CALIB >> 31) {
		*(unsigned int *)0xE000E010 = 0x07; /* 内部时钟，开启stk，开启stk中断 */
	} else {
		*(unsigned int *)0xE000E010 = 0x03; /* 外部时钟，开启stk，开启stk中断 */
	}

	return 0;
}


__asm void os_start(void)
{
	EXTERN running_tcb
	LDR	LR, =running_tcb      /* 起始任务栈 */
	LDR	LR, [LR]
	LDR	LR, [LR]
	LDMIA	LR!, {R4-R11}
	LDMIA	LR!, {R0-R3, R12} /* 初始化起始任务寄存器环境 */
	ADDS	LR, #(12)         /* 还原起始任务栈（LR/PC/PSR）*/
	MSR	PSP, LR
	MOVS	LR, #(3)
	MSR	CONTROL, LR       /* 切换到线程非特权模式 */
	LDR	LR, [R13, #(-8)]  /* 获取起始任务指针 */
	BX	LR                /* 进入起始任务 */
}

unsigned int init_stack[128/sizeof(unsigned int)];
void init_task(void)
{
	os_tick_init(10);
	while(1) {
	}
}
void os_init(void)
{
	int i;
	unsigned int *tsk_sp = (unsigned int *)init_stack + ((sizeof(init_stack)-sizeof(struct sos_tcb)) >> 2);
	
	for (i=0; i<PRIO_SUM; i++) {
		tsk_ready[i] = (struct sos_tcb *)0;
	}

	running_tcb = (struct sos_tcb *)tsk_sp;
	if ((int)tsk_sp % 8) { /* 堆栈双字对齐检查 */
		tsk_sp--;
	}
	*(--tsk_sp) = 0x01000000; /* xPSR Thumb mode */
	*(--tsk_sp) = (unsigned int)init_task; /* R15-PC */
	*(--tsk_sp) = 0x14141414; /* R14-LR */
	*(--tsk_sp) = 0x12121212; /* R12 */
	*(--tsk_sp) = 0x03030303; /* R3 */
	*(--tsk_sp) = 0x02020202; /* R2 */
	*(--tsk_sp) = 0x01010101; /* R1 */
	*(--tsk_sp) = 0x00000000; /* R0 */
	*(--tsk_sp) = 0x11111111; /* R11 */
	*(--tsk_sp) = 0x10101010; /* R10 */
	*(--tsk_sp) = 0x09090909; /* R9 */
	*(--tsk_sp) = 0x08080808; /* R8 */
	*(--tsk_sp) = 0x07070707; /* R7 */
	*(--tsk_sp) = 0x06060606; /* R6 */
	*(--tsk_sp) = 0x05050505; /* R5 */
	*(--tsk_sp) = 0x04040404; /* R4 */
	running_tcb->tsk_sp = (void *)tsk_sp;
	running_tcb->next = running_tcb;
	running_tcb->prev = running_tcb;

	running_tcb->tsk_prio = PRIO_SUM - 1;
	running_tcb->tsk_status = TSK_RUNNING;
}


int sys_create_tsk(void *tsk_func, void *tsk_stack, int stack_sz, char prio)
{
	struct sos_tcb *tcb;
	unsigned int *tsk_sp = (unsigned int *)tsk_stack + ((stack_sz-sizeof(struct sos_tcb)) >> 2);
	
	if (prio >= PRIO_SUM) {
		return -1;
	}

	tcb = (struct sos_tcb *)tsk_sp;
	if ((int)tsk_sp % 8) { /* 堆栈双字对齐检查 */
		tsk_sp--;
	}
	*(--tsk_sp) = 0x01000000; /* xPSR Thumb mode */
	*(--tsk_sp) = (unsigned int)tsk_func; /* R15-PC */
	*(--tsk_sp) = 0x14141414; /* R14-LR */
	*(--tsk_sp) = 0x12121212; /* R12 */
	*(--tsk_sp) = 0x03030303; /* R3 */
	*(--tsk_sp) = 0x02020202; /* R2 */
	*(--tsk_sp) = 0x01010101; /* R1 */
	*(--tsk_sp) = 0x00000000; /* R0 */
	*(--tsk_sp) = 0x11111111; /* R11 */
	*(--tsk_sp) = 0x10101010; /* R10 */
	*(--tsk_sp) = 0x09090909; /* R9 */
	*(--tsk_sp) = 0x08080808; /* R8 */
	*(--tsk_sp) = 0x07070707; /* R7 */
	*(--tsk_sp) = 0x06060606; /* R6 */
	*(--tsk_sp) = 0x05050505; /* R5 */
	*(--tsk_sp) = 0x04040404; /* R4 */
	
	tcb->tsk_sp = (void *)tsk_sp;
	if (tsk_ready[prio] == 0) {
		tsk_ready[prio] = tcb;
		tsk_ready[prio]->prev = tcb;
		tsk_ready[prio]->next = tcb;
	}
	tcb->prev = tsk_ready[prio]->prev;
	tcb->next = tsk_ready[prio];
	tcb->prev->next = tcb;
	tsk_ready[prio]->prev = tcb;

	tcb->tsk_prio = prio;
	tcb->tsk_status = TSK_READY;

	return (int)tcb; /* 任务控制块地址作为任务ID */
}

int sys_delete_tsk(struct sos_tcb *tcb)
{
	if (tcb == 0) { /* XXX 0 for NULL */
		return -1;
	}
	/* TODO: forbid delete init task */
	if (tcb->prev) {
		tcb->prev->next = tcb->next;
	}
	if (tcb->next) {
		tcb->next->prev = tcb->prev;
	}
	if (tsk_ready[tcb->tsk_prio] == tcb) {
		if ((tcb->prev == tcb->next)
		 && (tcb->prev == tcb)) {
			tsk_ready[tcb->tsk_prio] = 0;
		} else {
			tsk_ready[tcb->tsk_prio] = tcb->next;
		}
	}
	tcb->tsk_status = TSK_DELET;
	tcb->tsk_prio = PRIO_SUM - 1;

	if (tcb == running_tcb) {
		*(unsigned int *)NVIC_INT_CTL = pendsv_set;
	}
	return 0;
}

int sys_get_tsk_status(struct sos_tcb *tcb)
{
	return tcb->tsk_status;
}
int sys_get_self_tskid(void)
{
	return (int)running_tcb;
}


void sys_stk(void)
{
	*(unsigned int *)NVIC_INT_CTL = pendsv_set;
}

void get_hightest_task(void)
{
	int i;
	struct sos_tcb *tcb;
	
	for (i=0; i<=running_tcb->tsk_prio; i++) {
		if (tsk_ready[i]) {
			tcb = tsk_ready[i]; /* 取出高优先级任务 */
			if ((tsk_ready[i]->prev == tsk_ready[i]->next)
			 && (tsk_ready[i]->prev == tsk_ready[i])) {
				tsk_ready[i] = (struct sos_tcb *)0;
			} else {
				tsk_ready[i] = tcb->next;
				tcb->next->prev = tcb->prev;
				tcb->prev->next = tcb->next;
			}
			i = running_tcb->tsk_prio;
			if (tsk_ready[i] == 0) {
				if (running_tcb->tsk_status != TSK_DELET) {
					tsk_ready[i] = running_tcb;
					running_tcb->tsk_status = TSK_READY;
				}
			} else {
				if (running_tcb->tsk_status != TSK_DELET) {
					running_tcb->prev = tsk_ready[i]->prev;
					running_tcb->next = tsk_ready[i];
					tsk_ready[i]->prev->next = running_tcb;
					tsk_ready[i]->prev = running_tcb;
					running_tcb->tsk_status = TSK_READY;
				}
			}

			tcb->prev = tcb;
			tcb->next = tcb;
			tcb->tsk_status = TSK_RUNNING;
			running_tcb = tcb;
			break;
		}
	}
}

__asm void sys_psv(void)
{
	EXTERN running_tcb
	EXTERN get_hightest_task	

	LDR	R0, =running_tcb /* 调度前运行任务 */
	LDR	R0, [R0]
	PUSH	{R0, LR}
	BL	get_hightest_task
	POP	{R0, LR}
	LDR	R1, =running_tcb /* 调度后运行任务 */
	LDR	R1, [R1]
	LDR	R1, [R1]
	CMP	R0, R1
#if 0
	TST	LR, #4
	ITE	EQ
	MRSEQ	R2, MSP
	MRSNE	R2, PSP
#else
	MRSNE	R2, PSP
#endif
	STMDBNE   R2!, {R4-R11} /* 旧任务寄存器入栈 */
	STRNE     R2, [R0]      /* 旧任务栈指针 */
	LDMIANE   R1!, {R4-R11} /* 新任务寄存器出栈 */
	MSRNE     PSP, R1	      /* 新任务堆栈指针 */
	//MVNS    LR, #(2)      /* 准备返回线程非特权模式 */
	BX      LR
}


/* EOF */
