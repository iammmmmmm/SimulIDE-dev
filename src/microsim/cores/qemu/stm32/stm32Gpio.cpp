#include "stm32Gpio.h"

#include <cstdint>
#include <iostream>
#include <utility>

#include "stm32.h"
#include "stm32GpioReg.h"
using namespace stm32Gpio;
/**
 * @brief Gpio 类的构造函数
 * @param base_addr GPIO 端口的基地址
 * @param port_name 端口名称 (如 "Port A")
 */
Gpio::Gpio(const uint64_t base_addr, std::string port_name) : m_port_name(std::move(port_name)),
                                                              m_base_addr(base_addr) {
  // 初始化寄存器到复位值
  initialize_registers(port_name);
  qDebug() << "[GPIO] Initialized" << Gpio::getName().c_str()
      << "at base address 0x" << Qt::hex << m_base_addr;
}
/**
 * @brief 初始化 GPIO 寄存器到复位值
 */
void Gpio::initialize_registers(string portName) {
  // 端口配置低寄存器 (GPIOx_CFGLOW)，偏移地址 0x00
  m_registers[CFGLOW_OFFSET] = std::make_unique<stm32Gpio::GPIOx_CFGLOW>(CFGLOW_OFFSET,portName);
  // 端口配置高寄存器 (GPIOx_CFGHIG)，偏移地址 0x04
  m_registers[CFGHIG_OFFSET] = std::make_unique<stm32Gpio::GPIOx_CFGHIG>(CFGHIG_OFFSET,portName);
  // 端口输入数据寄存器 (GPIOx_IDATA)，偏移地址 0x08
  m_registers[IDATA_OFFSET] = std::make_unique<stm32Gpio::GPIOx_IDATA>(IDATA_OFFSET,portName);
  // 端口输出数据寄存器 (GPIOx_ODATA)，偏移地址 0x0C
  m_registers[ODATA_OFFSET] = std::make_unique<stm32Gpio::GPIOx_ODATA>(ODATA_OFFSET,portName);
  // 端口位设置/清除寄存器 (GPIOx_BSC)，偏移地址 0x10
  m_registers[BSC_OFFSET] = std::make_unique<stm32Gpio::GPIOx_BSC>(BSC_OFFSET,portName);
  // 端口位清除寄存器 (GPIOx_BC)，偏移地址 0x14
  m_registers[BC_OFFSET] = std::make_unique<stm32Gpio::GPIOx_BC>(BC_OFFSET,portName);
  // 端口配置锁定寄存器 (GPIOx_LOCK)，偏移地址 0x18
  m_registers[LOCK_OFFSET] = std::make_unique<stm32Gpio::GPIOx_LOCK>(LOCK_OFFSET,portName);
// =========================================================================
  // 寄存器写监控 (WR Monitor)
  // =========================================================================

  // --- 1. GPIOx_ODATA (0x0C): 端口输出数据寄存器
  // 写入 ODATA 会直接改变引脚的输出状态（如果引脚配置为推挽或开漏输出）
  auto* odata_reg = m_registers[ODATA_OFFSET]->find_field("ODATA");
  if (odata_reg) {
    odata_reg->write_callback = [](Register &reg, uint32_t new_field_value, void *user_data) {
      // TODO: 实现修改 GPIO 外部输出引脚状态的逻辑。

    };
  }

  // --- 2. GPIOx_BSC (0x10): 端口位设置/清除寄存器
  // 写入 BSC 寄存器会触发对 ODATA 寄存器的修改
  auto* bsc_reg = m_registers[BSC_OFFSET].get();
  if (bsc_reg) {
    // 监控 BS (Bit Set) 字段
    auto* bs_field = bsc_reg->find_field("BS");
    if (bs_field) {
      bs_field->write_callback = [this](Register &reg, uint32_t new_field_value, void *user_data) {
        // new_field_value 包含 BS[15:0] 中哪些位被设置为 1 (置位命令)
        // 任何写入 1 的位都会导致 ODATA 对应的位被设置为 1
        uint32_t set_mask = new_field_value;
        if (set_mask != 0) {
          // 非侵入式读取 ODATA 的当前值
          uint32_t current_odata = m_registers[ODATA_OFFSET]->get_field_value_non_intrusive("ODATA");
          // 计算新的 ODATA 值：当前值 | 置位掩码
          uint32_t new_odata = current_odata | set_mask;

          // 非侵入式更新 ODATA 的内部存储值
          m_registers[ODATA_OFFSET]->set_field_value_non_intrusive("ODATA", new_odata);

          // TODO: ODATA 被修改，实现修改 GPIO 外部输出引脚状态的逻辑。
        }
        Q_UNUSED(reg); Q_UNUSED(user_data);
      };
    }

    // 监控 BC (Bit Clear) 字段
    auto* bc_field = bsc_reg->find_field("BC");
    if (bc_field) {
      bc_field->write_callback = [this](Register &reg, uint32_t new_field_value, void *user_data) {
        // new_field_value 包含 BC[15:0] 中哪些位被设置为 1 (清除命令)
        // 任何写入 1 的位都会导致 ODATA 对应的位被清除为 0
        uint32_t clear_mask = new_field_value;
        if (clear_mask != 0) {
          // 非侵入式读取 ODATA 的当前值
          uint32_t current_odata = m_registers[ODATA_OFFSET]->get_field_value_non_intrusive("ODATA");
          // 计算新的 ODATA 值：当前值 & (~清除掩码)
          uint32_t new_odata = current_odata & (~clear_mask);

          // 非侵入式更新 ODATA 的内部存储值
          m_registers[ODATA_OFFSET]->set_field_value_non_intrusive("ODATA", new_odata);

          // TODO: ODATA 被修改，实现修改 GPIO 外部输出引脚状态的逻辑。
        }
        Q_UNUSED(reg); Q_UNUSED(user_data);
      };
    }
  }

  // --- 3. GPIOx_BC (0x14): 端口位清除寄存器 (简化版)
  // 写入 BC 寄存器会触发对 ODATA 寄存器的清除操作
  auto* bc_clear_reg = m_registers[BC_OFFSET].get();
  if (bc_clear_reg) {
    auto* bc_field_simple = bc_clear_reg->find_field("BC");
    if (bc_field_simple) {
      bc_field_simple->write_callback = [this](Register &reg, uint32_t new_field_value, void *user_data) {
        // new_field_value 包含 BC[15:0] 中哪些位被设置为 1 (清除命令)
        uint32_t clear_mask = new_field_value;
        if (clear_mask != 0) {
          // 非侵入式读取 ODATA 的当前值
          uint32_t current_odata = m_registers[ODATA_OFFSET]->get_field_value_non_intrusive("ODATA");
          // 计算新的 ODATA 值：当前值 & (~清除掩码)
          uint32_t new_odata = current_odata & (~clear_mask);

          // 非侵入式更新 ODATA 的内部存储值
          m_registers[ODATA_OFFSET]->set_field_value_non_intrusive("ODATA", new_odata);

          // TODO: ODATA 被修改，实现修改 GPIO 外部输出引脚状态的逻辑。
        }
        Q_UNUSED(reg); Q_UNUSED(user_data);
      };
    }
  }

  // --- 4. GPIOx_CFGLOW (0x00) / GPIOx_CFGHIG (0x04): 模式和功能配置
  // 写入配置寄存器会影响引脚的输入/输出/复用功能，需要外部 IO 模块进行适配。
  auto* cfglow_reg = m_registers[CFGLOW_OFFSET].get();
  auto* cfghig_reg = m_registers[CFGHIG_OFFSET].get();

  auto cfg_callback = [](Register &reg, uint32_t new_field_value, void *user_data) {
    // TODO: 实现更新 GPIO 外部模型配置的逻辑。

  };

  if (cfglow_reg) {
    for (int y = 0; y <= 7; ++y) {
      // 监控所有 MODE0-MODE7 和 CFG0-CFG7 字段
      cfglow_reg->find_field("MODE" + to_string(y))->write_callback = cfg_callback;
      cfglow_reg->find_field("CFG" + to_string(y))->write_callback = cfg_callback;
    }
  }
  if (cfghig_reg) {
    for (int y = 8; y <= 15; ++y) {
      // 监控所有 MODE8-MODE15 和 CFG8-CFG15 字段
      cfghig_reg->find_field("MODE" + to_string(y))->write_callback = cfg_callback;
      cfghig_reg->find_field("CFG" + to_string(y))->write_callback = cfg_callback;
    }
  }

  // --- 5. GPIOx_LOCK (0x18): 锁定寄存器
  // 监控 LOCKKEY 的写入序列，这可能会影响 CFGLOW/CFGHIG 的可写性
  auto* lock_reg = m_registers[LOCK_OFFSET].get();
  if (lock_reg) {
    // 监控 LOCKKEY 字段
    auto* lockkey_field = lock_reg->find_field("LOCKKEY");
    if (lockkey_field) {
      lockkey_field->write_callback = [](Register &reg, uint32_t new_field_value, void *user_data) {
        // TODO: 实现锁定序列检查和锁定/解锁配置寄存器的逻辑。

      };
    }
  }

  // TODO: 如果 GPIOx_IDATA 是输入，您还需要一个机制（例如外部 I/O 模块或回调）
  // 来**非侵入式**地更新 GPIOx_IDATA 寄存器的内部值，以模拟引脚的外部输入变化。
}
bool Gpio::is_clock_enabled(void *user_data) const {
  //a,b,c,d,e,均挂在ahpb2上,所以从AHPB2上查询
  const auto stm32_instance = static_cast<Stm32*>(user_data);
  const auto rcm_device = dynamic_cast<stm32Rcm::Rcm*>(stm32_instance->peripheral_registry->findDevice(RCM_BASE));
  string name="PAEN";
  switch (m_base_addr) {
    case GPIOA_BASE:
      name="PAEN";
      break;
      case GPIOB_BASE:
      name="PBEN";
      break;
      case GPIOC_BASE:
      name="PCEN";
      break;
      case GPIOD_BASE:
      name="PDEN";
      break;
      case GPIOE_BASE:
      name="PEEN";
      break;
      default:
      qWarning() << "[GPIO] Gpio::is_clock_enabled Error: Unknown base address 0x" << Qt::hex << m_base_addr<<"name be PAEN ";
      break;
  }
  return rcm_device->isAHPB2ClockEnabled(name);
}


// -----------------------------------------------------------------------------
// 读操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 GPIO 寄存器的读操作
 */
bool Gpio::handle_read(uc_engine *uc,
                       uint64_t address,
                       int size,
                       int64_t *read_value,
                       void *user_data) {
  // 仅支持 32 位读写
  if (size != 4) {
    qWarning() << "[GPIO R] Error: Only 32-bit (4 byte) access is supported for" << getName().c_str()<<Qt::endl;
    return false;
  }
  const auto offset = static_cast<uint32_t>(address - RCM_BASE);
  uint32_t stored_value = 0;
  if (m_registers.count(offset)) {
    stored_value = m_registers[offset]->read(user_data);
  } else {
    qWarning().noquote() << QString("   [GPIO R: 0x%1] 警告: 访问未注册寄存器!").arg(offset, 0, 16);
    *read_value = 0;
    return false;
  }
  *read_value = static_cast<int64_t>(stored_value);
  return true;
}


// -----------------------------------------------------------------------------
// 写操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 GPIO 寄存器的写操作
 */
bool Gpio::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) {
  // 仅支持 32 位读写
  if (size != 4) {
    qWarning("[GPIO W] Error: Only 32-bit (4 byte) access is supported for %s", getName().c_str());
    return false;
  }
  const auto offset = static_cast<uint32_t>(address - RCM_BASE);
  if (m_registers.count(offset)) {
    const auto new_value = static_cast<uint32_t>(value);
    m_registers[offset]->write(new_value,user_data);
  }else {
    qWarning().noquote() << QString("   [GPIO W: 0x%1] 警告: 访问未注册寄存器!").arg(offset, 0, 16);
    return false;
  }
  return true;
}
