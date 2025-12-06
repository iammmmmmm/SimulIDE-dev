#pragma once
#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <memory>
#include <functional>
#include <QDebug>
struct BitField;
using namespace std;
// =========================================================================
// 0. 回调函数定义
// =========================================================================
class Register; // 前向声明，确保 BitFieldWriteCallback 可以引用 Register

/**
 * @brief 位域级别的写入回调函数签名。
 *
 * @param reg 寄存器自身的引用。
 * @param new_field_value 写入该位域的新的值。回调函数在此处执行该位域变化引起的副作用。
 */
using BitFieldWriteCallback = std::function<void(Register &reg, uint32_t new_field_value)>;
/**
 * @brief 位域级别的读取回调函数签名。
 *
 * @param reg 寄存器自身的引用。
 * @param field 被读取的位域结构体的引用。
 * @param field_value_to_read 对即将返回的该位域的值的引用。回调函数可以在这里修改返回值，
 * 例如清除状态位（如果该位域是写清零型状态位）或返回特殊的只读值。
 */
using BitFieldReadCallback = std::function<void(Register &reg,
                                                const BitField &field,
                                                uint32_t &field_value_to_read)>;
// =========================================================================
// 1. 位域定义结构
// =========================================================================
enum class BitFieldAccess {
  RW, // 读写 (Read/Write)
  RO, // 只读 (Read Only) - 写入时被忽略
  WO, // 只写 (Write Only) - 写入有效，读取返回0 (需要在 read() 中处理)
  W1C, // 写1清零 (Write 1 to Clear) - 写入 1 清除该位，写入 0 无影响
  W0C // 写0清零 (Write 0 to Clear) - 写入 0 清除该位，写入 1 无影响 (较少见)
};
struct BitField {
  string name; // 位域名称，如 "HIRCEN"
  int start_bit; // 起始位 (从 0 开始)
  int width; // 宽度，对于单个位为 1
  BitFieldAccess access = BitFieldAccess::RW;
  // 存储位域值到描述的映射，例如 0->"关闭", 1->"打开"
  map<uint32_t, string> value_descriptions;
  //当此位域被修改时执行的回调函数
  BitFieldWriteCallback write_callback = nullptr;
  BitFieldReadCallback read_callback = nullptr;
};

// =========================================================================
// 2. 寄存器抽象基类
// =========================================================================

class Register {
  protected:
    uint32_t m_offset;
    uint32_t m_current_value = 0x00000000;
    string m_name;
    std::map<std::string, BitField *> m_field_map;

  public:
    // 存储该寄存器所有位域的定义
    vector<BitField> m_fields;
    static bool s_enable_debug_output;
    Register(uint32_t offset, string name) : m_offset(offset), m_name(std::move(name)) {
    }
    /**
     * @brief 构建位域查找映射。必须在所有 BitField 都被添加到 m_fields 后调用。
     */
    void build_field_map() {
      m_field_map.clear();
      for (auto &field: m_fields) {
        m_field_map[field.name] = &field; // 映射名称到 BitField 对象的地址
      }
    }
    /**
     * @brief 按名称查找位域 (O(log N) 性能)。
     * @param field_name 要查找的位域名称。
     * @return 找到的 BitField 引用指针，如果未找到则返回 nullptr。
     */
    BitField *find_field(const std::string &field_name) {
      auto it = m_field_map.find(field_name);
      if (it != m_field_map.end()) {
        return it->second; // 返回映射中存储的 BitField 指针
      }
      return nullptr; // 未找到
    }
    /**
 * @brief 三参数构造函数，用于设置初始复位值
 */
    Register(uint32_t offset, string name, uint32_t reset_value)
      : m_offset(offset), m_current_value(reset_value), m_name(std::move(name)) {
    }
    virtual ~Register();
    // 设置全局调试开关
    static void set_debug_output(bool enable) {
      s_enable_debug_output = enable;
    }
    // // 设置寄存器级别的读取回调
    // void set_read_callback(RegisterReadCallback callback) {
    //   m_read_callback = std::move(callback);
    // }
    // 获取寄存器偏移地址
    uint32_t get_offset() const { return m_offset; }

    // 模拟读取操作
    uint32_t read() {
      //位域级别读取回调
      for (const auto &field: m_fields) {
        if (field.read_callback) {
          // 提取当前位域值
          const uint32_t mask = ((1U << field.width) - 1) << field.start_bit;
          uint32_t field_value = (m_current_value & mask) >> field.start_bit;

          // 执行回调，回调可能会修改 field_value
          field.read_callback(*this, field, field_value);

          // 如果回调修改了值，则更新 m_current_value
          // 注意：有些位域读取后会清除标志位，所以需要更新 m_current_value
          const uint32_t new_field_shifted = field_value << field.start_bit;
          m_current_value = (m_current_value & ~mask) | (new_field_shifted & mask);
        }
      }
      uint32_t simulated_value = m_current_value; // 默认返回当前内部值（可能已被位域回调修改）

      // 处理只写 (WO) 属性：确保 WO 位的返回值是 0。
      for (const auto &field: m_fields) {
        if (field.access == BitFieldAccess::WO) {
          const uint32_t mask = ((1U << field.width) - 1) << field.start_bit;
          // 清除模拟值中 WO 位域的值，使其返回 0
          simulated_value &= ~mask;
        }
      }
      // 读取调试输出
      if (s_enable_debug_output) {
        debug_interpret_read(simulated_value);
      }
      return simulated_value;
    }

    /**
    * @brief 读取并返回指定位域的当前值（侵入式）。
    * * 注意：此方法会调用 Register::read()，因此会触发所有读回调和副作用。
    * * 模拟CPU/总线读取。
    * * @param field_name 要读取的位域名称。
    * @return uint32_t 位域的值。如果位域不存在，返回 0 并输出警告。
    */
    uint32_t get_field_value(const std::string &field_name) {
      // 1. 调用 read() 获取当前模拟的寄存器值，确保执行了所有回调。
      uint32_t current_reg_value = read();

      // 2. 查找位域定义
      const BitField *field = find_field(field_name);
      if (!field) {
        qDebug() << "Error: Attempted to read value from non-existent field" << field_name.c_str()
            << "in register" << m_name.c_str();
        return 0; // 返回 0 作为安全值
      }

      // 3. 计算掩码
      const uint32_t mask = ((1U << field->width) - 1) << field->start_bit;

      // 4. 提取位域值并返回
      return (current_reg_value & mask) >> field->start_bit;
    }

    /**
     * @brief 内部使用：读取指定位域的当前存储值，不触发任何读回调和副作用（非侵入式）。
     * 适用于模拟器内部逻辑（如时钟模块、状态检查）查询寄存器状态。
     * @param field_name 要读取的位域名称。
     * @return uint32_t 位域的值。如果位域不存在，返回 0 并输出警告。
     */
    uint32_t get_field_value_non_intrusive(const std::string &field_name) {
      // 1. 查找位域定义 (使用 const 版本的 find_field)
      const BitField *field = find_field(field_name);
      if (!field) {
        qDebug() << "Error: Attempted to read non-existent field" << field_name.c_str()
            << "in register" << m_name.c_str() << " (Non-intrusive read)";
        return 0; // 返回 0 作为安全值
      }

      // 2. 直接从 m_current_value 中提取值，不调用 read()
      const uint32_t mask = ((1U << field->width) - 1) << field->start_bit;

      return (m_current_value & mask) >> field->start_bit;
    }
    string getName() {
      return m_name;
    }

    // 模拟写入操作
    void write(uint32_t new_value) {
      const uint32_t old_value = m_current_value;
      uint32_t effective_value = old_value; // 从旧值开始，只修改允许被写入的部分

      // 1. 根据位域访问权限计算实际写入的值 (effective_value)
      for (const auto &field: m_fields) {
        const uint32_t mask = ((1U << field.width) - 1) << field.start_bit;
        switch (field.access) {
          case BitFieldAccess::RO:
            // 只读 (RO): 忽略写入。 effective_value 保留 old_value 的 RO 位。
            // 因为 effective_value 初始化为 old_value，所以这里不需要操作。
            break;

          case BitFieldAccess::RW:
          case BitFieldAccess::WO:
            // 读写 (RW) 或只写 (WO): 允许写入 new_value。
            // 清除 effective_value 中的旧值，并设置 new_value 中的新值
            effective_value = (effective_value & ~mask) | (new_value & mask);
            break;

          case BitFieldAccess::W1C: {
            // W1C 位的内部状态 (m_current_value) 不应被新写入的值影响。
            // 它只负责触发副作用。
            // 所以 effective_value 应该保持 old_value 在 W1C 位的状态，这里无需操作
            break;
          }
          case BitFieldAccess::W0C: {
            // 写 0 清零 (W0C)
            const uint32_t new_field_value = (new_value & mask) >> field.start_bit;

            if (new_field_value == 0) {
              // 如果写入的是 0，则清零 (设置 effective_value 中的该位为 0)
              effective_value &= ~mask;
            } else {
              // 如果写入的是 1，则保持不变
            }
          }
            break;
        }
      }
      // 更新内部值
      m_current_value = effective_value;

      //  遍历位域，检查变更并执行回调
      // 使用 auto& 遍历 m_fields，因为 write_callback 可能会在内部修改寄存器或其他状态
      for (auto &field: m_fields) {
        // 计算掩码
        const uint32_t mask = ((1U << field.width) - 1) << field.start_bit;
        uint32_t callback_value = 0; // 传递给回调的值
        bool should_call = false;
        switch (field.access) {
          case BitFieldAccess::RW:
          case BitFieldAccess::WO:
            // RW/WO: 如果写入的值导致实际寄存器值发生变化，则触发回调。
            // 提取更新后的寄存器值
            callback_value = (effective_value & mask) >> field.start_bit;
            // 只有当实际存储的值发生变化时才触发
            if (((old_value & mask) >> field.start_bit) != callback_value) {
              should_call = true;
            }
            break;
          case BitFieldAccess::W1C:
          case BitFieldAccess::W0C:
            // W1C/W0C (触发性): 只要软件写入了非零/零值，就应触发回调。
            // 提取软件尝试写入的值（即触发命令）
            callback_value = (new_value & mask) >> field.start_bit;
            // 检查 W1C: 写入 1 才触发。 检查 W0C: 写入 0 才触发。
            if (field.access == BitFieldAccess::W1C && callback_value == 1) {
              should_call = true;
            } else if (field.access == BitFieldAccess::W0C && callback_value == 0) {
              should_call = true;
            }
            break;

          case BitFieldAccess::RO:
            // RO (只读): 写入不触发任何回调。
            break;
        }

        // 执行回调
        if (should_call && field.write_callback) {
          field.write_callback(*this, callback_value);
        }
      }
      if (s_enable_debug_output) {
        debug_interpret_write(new_value);
      }
    }
    /**
    * @brief 修改特定位域的值（侵入式）,用于模拟硬件行为.
    * * 注意：此方法会调用 Register::write()，会触发所有读回调和副作用。
    * * 模拟CPU/总线读取。
   * @param field_name 要修改的位域名称。
   * @param new_field_value 要设置的位域值。
   */
    void set_field_value(const std::string &field_name, uint32_t new_field_value) {
      BitField *field = find_field(field_name);
      if (!field) {
        qDebug() << "Error: Field" << field_name.c_str() << "not found in register" << m_name.
            c_str();
        return;
      }

      // 1. 获取当前寄存器值
      const uint32_t old_value = m_current_value; // 使用内部值进行计算

      // 2. 计算掩码
      const uint32_t mask = ((1U << field->width) - 1) << field->start_bit;

      // 3. 检查新值是否在位域范围内
      if (new_field_value >= (1U << field->width)) {
        qDebug() << "Warning: Value" << new_field_value << "exceeds field" << field_name.c_str() <<
            "width" << field->width << "bits. Truncating.";
        new_field_value = (1U << field->width) - 1; // 截断到最大值
      }

      // 4. 计算新的完整寄存器值：清除旧值，设置新值
      const uint32_t new_field_shifted = new_field_value << field->start_bit;

      // 注意：这里需要计算出一个完整的 32 位值，然后调用 write()
      // RO, WO, W1C 逻辑将在 write() 内部处理。
      // 对于 set_field_value 这种“原子”操作，我们只需要构造出目标位域被修改的完整值。

      uint32_t value_to_write;
      // 对于 W1C 位域，如果想清零（写入 1），构造值应为 1。如果想保持（写入 0），构造值应为 0。
      if (field->access == BitFieldAccess::W1C) {
        // 如果目标是让它变成 0 (清零)，则构造一个写入 1 的值
        if (new_field_value == 0) {
          value_to_write = (old_value & ~mask) | mask; // 构造一个在这个位域写入 1 的值
        } else {
          // W1C 位域不能通过写入来设置 1，所以如果尝试设置非 0 值，则忽略，调用 write(old_value)
          // 为了简化并允许模拟器内部使用，我们先按普通写入处理，硬件的 W1C 逻辑由 write() 保证。
          // 实际硬件中，W1C 位域通常只能被清零。这里允许写入，但实际效果由 write() 决定。
          value_to_write = (old_value & ~mask) | (new_field_shifted & mask);
        }
      } else {
        // 对于 RW/WO/RO 位域，构造出目标位域被修改的完整值
        value_to_write = (old_value & ~mask) | (new_field_shifted & mask);
      }

      // 5. 调用完整的 write 方法来处理所有回调和读写限制。
      this->write(value_to_write);

      if (s_enable_debug_output) {
        qDebug().noquote() << QString("--- (原子写入) 字段 %1: %2 更新完成 ---")
            .arg(m_name.c_str()).arg(field_name.c_str());
      }
    }
    /**
        * @brief 专门用于模拟器内部逻辑，修改特定位域的值，不触发调试输出和回调。
        * 适用于模拟硬件的副作用，例如在启用时钟后设置就绪标志。
        * @param field_name 要修改的位域名称。
        * @param value_to_set 要设置的位域值。
        */
    void set_field_value_non_intrusive(const std::string &field_name, uint32_t value_to_set) {
      BitField *field = find_field(field_name);
      if (!field) {
        qDebug() << "Error: Field" << field_name.c_str() << "not found in register" << m_name.
            c_str();
        return;
      }

      // 1. 计算掩码
      const uint32_t mask = ((1U << field->width) - 1) << field->start_bit;

      // 检查新值是否在位域范围内
      if (value_to_set >= (1U << field->width)) {
        qDebug() << "Warning: Value" << value_to_set << "exceeds field" << field_name.c_str() <<
            "width" << field->width << "bits.";
        value_to_set = (1U << field->width) - 1; // 截断到最大值
      }

      const uint32_t new_field_shifted = value_to_set << field->start_bit;

      // 2. 清除旧值并设置新值
      m_current_value = (m_current_value & ~mask) | (new_field_shifted & mask);
      if (s_enable_debug_output) {
        qDebug().noquote() << "模拟器内部写入字段：" << field_name.c_str() << "，值：0x"<<Qt::hex << new_field_shifted;
      }
      // 注意：这里不执行 write 回调，也不执行 debug 输出，因为这是模拟的副作用，而不是一次真实地写操作。
    }

  private:
    // 位域解析和显示逻辑 (仅用于调试)
    void debug_interpret_write(uint32_t value) const {
      // 显示写入头信息
      qDebug().noquote() << "\n--- 写入寄存器: "
          << m_name.c_str() // 将 std::string 转换为 C 字符串
          << QString(" (Offset: 0x%1) ---")
          .arg(m_offset, 2, 16, QChar('0')); // Qt 格式化: 2位, 十六进制, 填充'0'

      qDebug().noquote() << QString("  实际值: 0x%1") // 更改为 实际值
          .arg(value, 8, 16, QChar('0')); // Qt 格式化: 8位, 十六进制, 填充'0'
      // 获取寄存器内部的当前值 (在 write() 逻辑计算后, 但在 debug 输出前)
    // 因为 debug_interpret_write 是在 m_current_value 被更新后调用的，所以此时 m_current_value 是最终结果。
    const uint32_t internal_value = m_current_value;
      for (const auto &field: m_fields) {
        // 计算掩码 (Mask)
        uint32_t mask = ((1U << field.width) - 1) << field.start_bit;

        // 提取该位域的值
        uint32_t field_value ;
        if (field.access == BitFieldAccess::RO || field.access == BitFieldAccess::W1C) {
          // 对于 RO 位域（只读，写入被忽略）和 W1C 位域（写入不改变状态），
          // 应该显示其在寄存器中实际保留的值 (internal_value)。
          field_value = (internal_value & mask) >> field.start_bit;
        } else {
          // 对于 RW, WO, W0C 位域，显示固件尝试写入的值 (value)，以体现固件意图。
          field_value = (value & mask) >> field.start_bit;
        }
        // 构建位域范围字符串
        QString bit_range = QString::number(field.start_bit);
        if (field.width > 1) {
          bit_range += "-" + QString::number(field.start_bit + field.width - 1);
        }
        // 标记访问权限
        QString access_str;
        switch (field.access) {
          case BitFieldAccess::RO: access_str = "(RO)"; break;
          case BitFieldAccess::WO: access_str = "(WO)"; break;
          case BitFieldAccess::W1C: access_str = "(W1C)"; break;
          case BitFieldAccess::RW: default: access_str = "(RW)"; break;
        }

        // 基础输出
        QString output_line = QString("  > **位域 %1: %2 %3** ").arg(bit_range, field.name.c_str(), access_str);

        // 查找描述信息
        if (field.value_descriptions.count(field_value)) {
          output_line += QString(" (值: %1) -> %2").arg(field_value)
              .arg(field.value_descriptions.at(field_value).c_str());
        } else {
          // 如果没有特定描述，直接显示值
          output_line += QString(" (值: %1)").arg(field_value);
        }

        qDebug().noquote() << output_line;
      }
    }
    // 位域解析和显示逻辑
    void debug_interpret_read(uint32_t value) const {
      // 显示读取头信息
      qDebug().noquote() << "\n--- 读取寄存器: "
          << m_name.c_str()
          << QString(" (Offset: 0x%1) ---")
          .arg(m_offset, 2, 16, QChar('0'));

      qDebug().noquote() << QString("  返回值: 0x%1")
          .arg(value, 8, 16, QChar('0'));

      for (const auto &field: m_fields) {
        // 计算掩码 (Mask)
        uint32_t mask = ((1U << field.width) - 1) << field.start_bit;

        // 提取该位域的值
        uint32_t field_value = (value & mask) >> field.start_bit;

        // 构建位域范围字符串
        QString bit_range = QString::number(field.start_bit);
        if (field.width > 1) {
          bit_range += "-" + QString::number(field.start_bit + field.width - 1);
        }
        // 标记访问权限
        QString access_str;
        switch (field.access) {
          case BitFieldAccess::RO: access_str = "(RO)"; break;
          case BitFieldAccess::WO: access_str = "(WO)"; break;
          case BitFieldAccess::W1C: access_str = "(W1C)"; break;
          case BitFieldAccess::RW: default: access_str = "(RW)"; break;
        }
        // 基础输出
        QString output_line = QString("  > **位域 %1: %2 %3** ").arg(bit_range, field.name.c_str(), access_str);
        // 查找描述信息
        if (field.value_descriptions.count(field_value)) {
          output_line += QString(" (当前值: %1) -> %2").arg(field_value)
              .arg(field.value_descriptions.at(field_value).c_str());
        } else {
          // 如果没有特定描述，直接显示值
          output_line += QString(" (当前值: %1)").arg(field_value);
        }
        qDebug().noquote() << output_line;
      }
    }
};
