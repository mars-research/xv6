// Definitions of functions specific to x86_64
struct rtcdata;
struct spinlock;

// lapic.c
void            cmostime(struct rtcdate *r);
int             lapicid(void);
extern volatile uint*    lapic;
void            lapiceoi(void);
void            lapicinit(void);
void            lapicstartap(uchar, uint);
void            microdelay(int);

// spinlock.c
void            pushcli(void);
void            popcli(void);
