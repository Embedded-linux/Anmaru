#ifndef CORE_CM4_H
#define CORE_CM4_H

#include <stdint.h>

/* IO definitions */
#define __I     volatile const
#define __O     volatile
#define __IO    volatile

/* Core intrinsic functions */
static inline void __DSB(void) {
    __asm volatile ("dsb 0xF":::"memory");
}

static inline void __ISB(void) {
    __asm volatile ("isb 0xF":::"memory");
}

static inline void __DMB(void) {
    __asm volatile ("dmb 0xF":::"memory");
}

static inline void __NOP(void) {
    __asm volatile ("nop");
}

static inline uint32_t __get_MSP(void) {
    uint32_t result;
    __asm volatile ("MRS %0, msp" : "=r" (result));
    return result;
}

static inline void __set_MSP(uint32_t topOfMainStack) {
    __asm volatile ("MSR msp, %0" : : "r" (topOfMainStack));
}

static inline uint32_t __get_PSP(void) {
    uint32_t result;
    __asm volatile ("MRS %0, psp" : "=r" (result));
    return result;
}

static inline void __set_PSP(uint32_t topOfProcStack) {
    __asm volatile ("MSR psp, %0" : : "r" (topOfProcStack));
}

static inline void __set_BASEPRI(uint32_t value) {
    __asm volatile ("MSR basepri, %0" : : "r" (value) : "memory");
}

static inline void __enable_irq(void) {
    __asm volatile ("cpsie i" : : : "memory");
}

static inline void __disable_irq(void) {
    __asm volatile ("cpsid i" : : : "memory");
}

static inline uint32_t __get_PRIMASK(void) {
    uint32_t result;
    __asm volatile ("MRS %0, primask" : "=r" (result));
    return result;
}

static inline void __set_PRIMASK(uint32_t priMask) {
    __asm volatile ("MSR primask, %0" : : "r" (priMask));
}

/* System Control Block (SCB) - Complete structure */
typedef struct {
    __I  uint32_t CPUID;
    __IO uint32_t ICSR;
    __IO uint32_t VTOR;
    __IO uint32_t AIRCR;
    __IO uint32_t SCR;
    __IO uint32_t CCR;
    __IO uint8_t  SHP[12];
    __IO uint32_t SHCSR;
    __IO uint32_t CFSR;
    __IO uint32_t HFSR;
    __IO uint32_t DFSR;
    __IO uint32_t MMFAR;
    __IO uint32_t BFAR;
    __IO uint32_t AFSR;
    __I  uint32_t PFR[2];
    __I  uint32_t DFR;
    __I  uint32_t ADR;
    __I  uint32_t MMFR[4];
    __I  uint32_t ISAR[5];
    uint32_t RESERVED0[5];
    __IO uint32_t CPACR;
    uint32_t RESERVED1[93];
    __O  uint32_t STIR;
    uint32_t RESERVED2[15];
    __I  uint32_t MVFR0;
    __I  uint32_t MVFR1;
    __I  uint32_t MVFR2;
    uint32_t RESERVED3;
    __O  uint32_t ICIALLU;
    uint32_t RESERVED4;
    __O  uint32_t ICIMVAU;
    __O  uint32_t DCIMVAC;
    __O  uint32_t DCISW;
    __O  uint32_t DCCMVAU;
    __O  uint32_t DCCMVAC;
    __O  uint32_t DCCSW;
    __O  uint32_t DCCIMVAC;
    __O  uint32_t DCCISW;
    uint32_t RESERVED5[6];
    __IO uint32_t ITCMCR;
    __IO uint32_t DTCMCR;
    __IO uint32_t AHBPCR;
    __IO uint32_t CACR;
    __IO uint32_t AHBSCR;
    uint32_t RESERVED6;
    __IO uint32_t ABFSR;
    uint32_t RESERVED7[55];
    __IO uint32_t IEBR0;
    __IO uint32_t IEBR1;
    __IO uint32_t D_BCR[8];
    __IO uint32_t D_BWC[8];
    uint32_t RESERVED8[57];
    __IO uint32_t CSSELR;
} SCB_Type;

/* SysTick */
typedef struct {
    __IO uint32_t CTRL;
    __IO uint32_t LOAD;
    __IO uint32_t VAL;
    __I  uint32_t CALIB;
} SysTick_Type;

/* NVIC */
typedef struct {
    __IO uint32_t ISER[8];
    uint32_t RESERVED0[24];
    __IO uint32_t ICER[8];
    uint32_t RESERVED1[24];
    __IO uint32_t ISPR[8];
    uint32_t RESERVED2[24];
    __IO uint32_t ICPR[8];
    uint32_t RESERVED3[24];
    __IO uint32_t IABR[8];
    uint32_t RESERVED4[56];
    __IO uint8_t  IP[240];
    uint32_t RESERVED5[644];
    __O  uint32_t STIR;
} NVIC_Type;

/* MPU */
typedef struct {
    __I  uint32_t TYPE;
    __IO uint32_t CTRL;
    __IO uint32_t RNR;
    __IO uint32_t RBAR;
    __IO uint32_t RASR;
} MPU_Type;

/* Core peripherals */
#define SCS_BASE            (0xE000E000UL)
#define SysTick_BASE        (SCS_BASE + 0x0010UL)
#define NVIC_BASE           (SCS_BASE + 0x0100UL)
#define SCB_BASE            (SCS_BASE + 0x0D00UL)
#define SCnSCB_BASE         (SCS_BASE + 0x0000UL)
#define SCnSCB              ((SCnSCB_Type *) SCnSCB_BASE)

typedef struct {
    uint32_t RESERVED0[1];
    __I  uint32_t ICTR;
    __IO uint32_t ACTLR;
} SCnSCB_Type;
#define MPU_BASE            (SCS_BASE + 0x0D90UL)

#define SCB                 ((SCB_Type *) SCB_BASE)
#define SysTick             ((SysTick_Type *) SysTick_BASE)
#define NVIC                ((NVIC_Type *) NVIC_BASE)
#define MPU                 ((MPU_Type *) MPU_BASE)

/* SCB bits */
#define SCB_AIRCR_VECTKEY_Pos    16U
#define SCB_AIRCR_SYSRESETREQ_Msk (1UL << 2U)
#define SCB_SHCSR_USGFAULTENA_Msk (1UL << 18U)
#define SCB_SHCSR_BUSFAULTENA_Msk (1UL << 17U)
#define SCB_SHCSR_MEMFAULTENA_Msk (1UL << 16U)
#define SCB_CFSR_MSTKERR_Msk    (1UL << 4U)
#define SCB_CFSR_STKERR_Msk     (1UL << 12U)
#define SCB_CFSR_DIVBYZERO_Msk  (1UL << 25U)
#define SCB_CFSR_UNALIGNED_Msk  (1UL << 24U)
#define SCB_CCR_STKALIGN_Msk    (1UL << 9U)
#define SCB_CCR_BP_Msk          (1UL << 18U)
#define SCB_CCR_IC_Msk          (1UL << 17U)
#define SCB_CCR_DC_Msk          (1UL << 16U)
#define SCB_ACTLR_DISDEFWBUF_Msk (1UL << 1U)

/* SysTick bits */
#define SysTick_CTRL_CLKSOURCE_Msk (1UL << 2U)
#define SysTick_CTRL_ENABLE_Msk    (1UL << 0U)

/* MPU bits */
#define MPU_CTRL_ENABLE_Msk     (1UL << 0U)
#define MPU_CTRL_PRIVDEFENA_Msk (1UL << 2U)
#define MPU_RASR_ENABLE_Msk     (1UL << 0U)
#define MPU_RASR_SIZE_Pos       1U
#define MPU_RASR_AP_Pos         24U
#define MPU_RASR_C_Pos          17U
#define MPU_RASR_S_Pos          18U
#define MPU_RASR_B_Pos          16U
#define MPU_RASR_XN_Pos         28U

/* FPU bits */
typedef struct {
    __IO uint32_t FPCCR;
    __IO uint32_t FPCAR;
    __IO uint32_t FPDSCR;
    __I  uint32_t MVFR0;
    __I  uint32_t MVFR1;
} FPU_Type;

#define FPU_BASE            (SCS_BASE + 0x0F30UL)
#define FPU                 ((FPU_Type *) FPU_BASE)
#define FPU_FPCCR_ASPEN_Msk (1UL << 31U)
#define FPU_FPCCR_LSPEN_Msk (1UL << 30U)

/* IRQ Numbers */
typedef enum {
    NonMaskableInt_IRQn   = -14,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn        = -11,
    UsageFault_IRQn      = -10,
    SVCall_IRQn          = -5,
    DebugMonitor_IRQn    = -4,
    PendSV_IRQn          = -2,
    SysTick_IRQn         = -1,
    WWDG_IRQn            = 0,
    PVD_IRQn             = 1,
    EXTI0_IRQn           = 6
    USART1_IRQn          = 37,
    USART2_IRQn          = 38,
    USART3_IRQn          = 39,
    UART4_IRQn           = 52,
    UART5_IRQn           = 53,
    USART6_IRQn          = 71,
} IRQn_Type;

/* NVIC functions */
static inline void NVIC_SetPriorityGrouping(uint32_t PriorityGroup) {
    uint32_t reg_value;
    reg_value = SCB->AIRCR;
    reg_value &= ~((uint32_t)0xFFFF0700U);
    reg_value |= ((uint32_t)0x5FAUL << 16U) | ((PriorityGroup & 0x07U) << 8U);
    SCB->AIRCR = reg_value;
}

static inline uint32_t NVIC_GetPriorityGrouping(void) {
    return ((SCB->AIRCR & (7UL << 8U)) >> 8U);
}

static inline void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority) {
    if ((int32_t)IRQn < 0) {
        SCB->SHP[(((uint32_t)(int32_t)IRQn) & 0xFUL) - 4UL] = 
            (uint8_t)((priority << (8U - 4U)) & (uint32_t)0xFFUL);
    } else {
        NVIC->IP[(uint32_t)IRQn] = (uint8_t)((priority << (8U - 4U)) & (uint32_t)0xFFUL);
    }
}

static inline void NVIC_EnableIRQ(IRQn_Type IRQn) {
    NVIC->ISER[(((uint32_t)(int32_t)IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)(int32_t)IRQn) & 0x1FUL));
}

static inline void NVIC_SystemReset(void) {
    __DSB();
    SCB->AIRCR = (0x5FAUL << SCB_AIRCR_VECTKEY_Pos) | SCB_AIRCR_SYSRESETREQ_Msk;
    __DSB();
    for(;;) {
        __NOP();
    }
}

/* Cache functions - simplified for Cortex-M4 (no cache) */
static inline void SCB_EnableICache(void) {
    /* Cortex-M4 doesn't have instruction cache, this is a no-op */
    __DSB();
    __ISB();
}

static inline void SCB_EnableDCache(void) {
    /* Cortex-M4 doesn't have data cache, this is a no-op */
    __DSB();
    __ISB();
}

/* Get FPSCR */
static inline uint32_t __get_FPSCR(void) {
    uint32_t result;
    __asm volatile ("VMRS %0, fpscr" : "=r" (result));
    return result;
}

/* Set FPSCR */
static inline void __set_FPSCR(uint32_t fpscr) {
    __asm volatile ("VMSR fpscr, %0" : : "r" (fpscr));
}

#endif /* CORE_CM4_H */
