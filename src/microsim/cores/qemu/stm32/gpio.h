#pragma once

#include <string>
#include <unicorn/unicorn.h>
#include "peripheral_factory.h"

// GPIO 外设基地址宏定义
#define GPIOA_BASE            ((uint32_t)0x40010800) // Port A 基地址 (APB2 总线)
#define GPIOB_BASE            ((uint32_t)0x40010C00) // Port B 基地址 (APB2 总线)
#define GPIOC_BASE            ((uint32_t)0x40011000) // Port C 基地址 (APB2 总线)
#define GPIOD_BASE            ((uint32_t)0x40011400) // Port D 基地址 (APB2 总线)
#define GPIOE_BASE            ((uint32_t)0x40011800) // Port E 基地址 (APB2 总线)

// 单个 GPIO 端口的地址范围 (0x1C + 0x4 = 0x20)
#define GPIO_REG_SIZE         ((uint32_t)0x20)

// --- GPIO 寄存器偏移地址宏定义 (Offset) ---

#define GPIO_CFGLOW_OFFSET    ((uint32_t)0x00) // 端口配置低寄存器 (GPIOx_CFGLOW)
#define GPIO_CFGHIG_OFFSET    ((uint32_t)0x04) // 端口配置高寄存器 (GPIOx_CFGHIG)
#define GPIO_IDATA_OFFSET     ((uint32_t)0x08) // 端口输入数据寄存器 (GPIOx_IDATA)
#define GPIO_ODATA_OFFSET     ((uint32_t)0x0C) // 端口输出数据寄存器 (GPIOx_ODATA)
#define GPIO_BSC_OFFSET       ((uint32_t)0x10) // 端口位设置/清除寄存器 (GPIOx_BSC)
#define GPIO_BC_OFFSET        ((uint32_t)0x14) // 端口位清除寄存器 (GPIOx_BC)
#define GPIO_LOCK_OFFSET      ((uint32_t)0x18) // 端口配置锁定寄存器 (GPIOx_LOCK)


// --- 结构体定义 ---

/**
 * @brief GPIO 端口寄存器定义
 * * 适用于 Port A 到 Port E (GPIOx)
 */
typedef struct {
  volatile uint32_t CFGLOW;     /**< 0x00: 端口配置低寄存器 (GPIOx_CFGLOW), Pin 0-7 */
  volatile uint32_t CFGHIG;     /**< 0x04: 端口配置高寄存器 (GPIOx_CFGHIG), Pin 8-15 */
  volatile const uint32_t IDATA;/**< 0x08: 端口输入数据寄存器 (GPIOx_IDATA), 只读 */
  volatile uint32_t ODATA;      /**< 0x0C: 端口输出数据寄存器 (GPIOx_ODATA) */
  volatile uint32_t BSC;        /**< 0x10: 端口位设置/清除寄存器 (GPIOx_BSC), 只写 */
  volatile uint32_t BC;         /**< 0x14: 端口位清除寄存器 (GPIOx_BC), 只写 */
  volatile uint32_t LOCK;       /**< 0x18: 端口配置锁定寄存器 (GPIOx_LOCK) */
  volatile uint32_t RESERVED;   /**< 0x1C: 保留/填充, 使结构体大小为 0x20 */
} GPIO_TypeDef;


// --- GPIO 外设模式和功能宏定义 (用于 CFGLOW/CFGHIG 寄存器) ---

// MODEy[1:0] (位 0, 4, 8, ... 28)
#define GPIO_MODE_INPUT       ((uint32_t)0x0) // 00: 输入模式
#define GPIO_MODE_OUTPUT_10M  ((uint32_t)0x1) // 01: 输出模式, 10MHz
#define GPIO_MODE_OUTPUT_2M   ((uint32_t)0x2) // 10: 输出模式, 2MHz
#define GPIO_MODE_OUTPUT_50M  ((uint32_t)0x3) // 11: 输出模式, 50MHz

// CFGy[1:0] (位 2, 6, 10, ... 30) - 输入模式时 (MODE=00)
#define GPIO_CFG_IN_ANALOG    ((uint32_t)0x0) // 00: 模拟输入模式
#define GPIO_CFG_IN_FLOATING  ((uint32_t)0x1) // 01: 浮空输入模式 (复位值)
#define GPIO_CFG_IN_PULLUPDOWN ((uint32_t)0x2) // 10: 上拉/下拉输入模式
#define GPIO_CFG_IN_RESERVED  ((uint32_t)0x3) // 11: 保留

// CFGy[1:0] (位 2, 6, 10, ... 30) - 输出模式时 (MODE>00)
#define GPIO_CFG_OUT_PP       ((uint32_t)0x0) // 00: 通用推挽输出模式
#define GPIO_CFG_OUT_OD       ((uint32_t)0x1) // 01: 通用开漏输出模式
#define GPIO_CFG_OUT_AF_PP    ((uint32_t)0x2) // 10: 复用功能推挽输出模式
#define GPIO_CFG_OUT_AF_OD    ((uint32_t)0x3) // 11: 复用功能开漏输出模式

// 位操作宏
#define GPIO_PIN_SET          ((uint32_t)0x1)
#define GPIO_PIN_RESET        ((uint32_t)0x0)

// --- RCM 时钟使能位宏定义 (用于 RCM_APB2CLKEN) ---

#define RCM_APB2CLKEN_AFIOEN   ((uint32_t)0x00000001) // AFIOEN 位 0
#define RCM_APB2CLKEN_PAEN     ((uint32_t)0x00000004) // PAEN 位 2 (Port A)
#define RCM_APB2CLKEN_PBEN     ((uint32_t)0x00000008) // PBEN 位 3 (Port B)
#define RCM_APB2CLKEN_PCEN     ((uint32_t)0x00000010) // PCEN 位 4 (Port C)
#define RCM_APB2CLKEN_PDEN     ((uint32_t)0x00000020) // PDEN 位 5 (Port D)
#define RCM_APB2CLKEN_PEEN     ((uint32_t)0x00000040) // PEEN 位 6 (Port E)


/**
 * @brief GPIO 外设模拟类定义
 */
class Gpio : public PeripheralDevice {
    GPIO_TypeDef m_registers{};
    std::string m_port_name; // 例如 "Port A", "Port B"
    uint64_t m_base_addr;    // 端口基地址
    void initialize_registers();
    // 检查 RCM 中对应 GPIO 端口的时钟是否开启
    bool is_clock_enabled() const;

public:
    /**
     * @brief 构造函数
     * @param base_addr GPIO 端口的基地址 (如 GPIOA_BASE)
     * @param port_name 端口名称 (如 "Port A")
     */
    Gpio(uint64_t base_addr, const std::string& port_name);

    // PeripheralDevice 接口实现
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) override;
    uint64_t getBaseAddress() override { return m_base_addr; }
    std::string getName() override { return "GPIO (" + m_port_name + ")"; }
    uint64_t getSize() override { return GPIO_REG_SIZE; }

};