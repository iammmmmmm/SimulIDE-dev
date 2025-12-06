#pragma once

#include <string>
#include <unicorn/unicorn.h>
#include "peripheral_factory.h" // 假设 Afio 继承自 PeripheralDevice

// --- AFIO 外设基地址宏定义 ---

// APB2总线 0x40010000 AFIO
#define AFIO_BASE_ADDR           ((uint32_t)0x40010000)
// 寄存器地址范围 0x00 到 0x14, 总大小为 0x18
#define AFIO_REG_SIZE            ((uint32_t)0x18)

// --- AFIO 寄存器偏移地址宏定义 (Offset) ---

#define AFIO_EVCTRL_OFFSET       ((uint32_t)0x00) // 事件控制寄存器
#define AFIO_REMAP1_OFFSET       ((uint32_t)0x04) // 复用重映射寄存器1
#define AFIO_EINTSEL1_OFFSET     ((uint32_t)0x08) // 外部中断输入源选择寄存器1
#define AFIO_EINTSEL2_OFFSET     ((uint32_t)0x0C) // 外部中断输入源选择寄存器2
#define AFIO_EINTSEL3_OFFSET     ((uint32_t)0x10) // 外部中断输入源选择寄存器3
#define AFIO_EINTSEL4_OFFSET     ((uint32_t)0x14) // 外部中断输入源选择寄存器4

// --- 结构体定义 ---

/**
 * @brief AFIO 寄存器定义
 * * 内存映射结构体，用于在模拟器中保存 AFIO 寄存器的状态
 */
typedef struct {
  volatile uint32_t EVCTRL;       /**< 0x00: 事件控制寄存器 (复位值: 0x00000000) */
  volatile uint32_t REMAP1;       /**< 0x04: 复用重映射寄存器1 (复位值: 0x00000000) */
  volatile uint32_t EINTSEL1;     /**< 0x08: 外部中断输入源选择寄存器1 (复位值: 0x00000000) */
  volatile uint32_t EINTSEL2;     /**< 0x0C: 外部中断输入源选择寄存器2 (复位值: 0x00000000) */
  volatile uint32_t EINTSEL3;     /**< 0x10: 外部中断输入源选择寄存器3 (复位值: 0x00000000) */
  volatile uint32_t EINTSEL4;     /**< 0x14: 外部中断输入源选择寄存器4 (复位值: 0x00000000) */
} AFIO_TypeDef;

// --- AFIO 关键位定义 (用于 REMAP1) ---

#define AFIO_REMAP1_SWJCFG_MASK  ((uint32_t)0x07000000) // SWJCFG 位 26:24 (只写)
#define AFIO_REMAP1_SWJCFG_SHIFT ((uint32_t)24)

/**
 * @brief AFIO 外设模拟类定义
 */
class Afio : public PeripheralDevice {
private:
    AFIO_TypeDef m_registers{};
    std::string m_peripheral_name = "AFIO";
    uint64_t m_base_addr = AFIO_BASE_ADDR;

    // RCM_APB2CLKEN 地址 (假设与 GPIO 共用 RCM 模块)
    // 假设 RCM_BASE_ADDR = 0x40021000, APB2CLKEN_OFFSET = 0x0C (需根据实际手册确认)
    // 这里只使用 gpio.h 中定义的 AFIOEN 位
    #define RCM_APB2CLKEN_AFIOEN   ((uint32_t)0x00000001) // AFIOEN 位 0
    #define APB2CLKEN_ADDR     ((uint64_t)0x4002100C)

    void initialize_registers();

    // 检查 AFIO 时钟是否开启
    bool is_clock_enabled(uc_engine *uc) const;

public:
    /**
     * @brief 构造函数
     */
    Afio();

    // PeripheralDevice 接口实现
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) override;
    uint64_t getBaseAddress() override { return m_base_addr; }
    std::string getName() override { return m_peripheral_name; }
    uint64_t getSize() override { return AFIO_REG_SIZE; }
};