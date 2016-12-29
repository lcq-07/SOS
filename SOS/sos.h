/**
 * @file    sos.h
 * @brief   sos.h
 * @author  nuc0707#163#com
 * @date    2016-12-20 20:55
 * @par     append in the following formate
 * year-month-day description
 */

#ifndef _SOS_H_
#define _SOS_H_

#define PRIO_SUM 8

#define TSK_READY 0
#define TSK_RUNNING 1
#define TSK_DELET 2
#define TSK_WAIT_OR  

struct sos_tcb {
	void *tsk_sp; /* 任务堆栈指针 */
	struct sos_tcb *next;
	struct sos_tcb *prev;
	char tsk_status; /* 任务状态 */
	char tsk_event;
	char tsk_prio; /* 任务优先级 */
	char tsk_rsv2;
};

extern void setup_sp(void);
extern void tsk_sch(void);
extern void os_init(void);
extern void os_start(void);
extern void sys_psv(void);
extern void sys_stk(void);
extern void sys_svc(void);
extern int __svc(0x00) os_tick_init(int ms);
extern int __svc(0x01) os_create_tsk(void *tsk_func, void *tsk_stack, int stack_sz, char prio);
extern int __svc(0x02) os_delete_tsk(int tsk_id);
extern int __svc(0x03) os_get_tsk_status(int tsk_id);
extern int __svc(0x04) os_get_self_tskid(void);
#if 0
extern int __svc(0x03) os_suspend_tsk(void *tsk_func, void *tsk_stack, int stack_sz);
extern int __svc(0x04) os_resume_tsk(void *tsk_func, void *tsk_stack, int stack_sz);
#endif
extern int __svc(0xFF) call_svc_n(int svc_r0, int svc_r1, int svc_r2, int svc_r3);

extern int sys_stk_init(int ms);
extern int sys_create_tsk(void *tsk_func, void *tsk_stack, int stack_sz, char prio);
extern int sys_delete_tsk(struct sos_tcb *tcb);
extern int sys_get_tsk_status(struct sos_tcb *tcb);
extern int sys_get_self_tskid(void);



#endif /* _SOS_H_ */
/* EOF */
