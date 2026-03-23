#include "risk.h"
#include <algorithm> // std::max — 返回两个值中较大的那个
#include <iostream>  // std::cout — 打印熔断提示

// ─────────────────────────────────────────────
// createRiskManager 实现
// ─────────────────────────────────────────────
RiskManager createRiskManager(double initialEquity, double limit) {
  RiskManager rm;
  rm.peakEquity = initialEquity; // 初始峰值 = 初始资金
  rm.drawdown = 0.0;             // 初始回撤为 0
  rm.maxDrawdown = 0.0;          // 初始历史最大回撤为 0
  rm.halted = false;             // 初始状态：正常运行，未熔断
  rm.limit = limit;              // 保存回撤阈值
  return rm;
}

// ─────────────────────────────────────────────
// updateRisk 实现
// ─────────────────────────────────────────────
void updateRisk(RiskManager &rm, double currentEquity,
                const std::string &date) {

  // ── 步骤 1：更新峰值 ──────────────────────────────────
  // std::max(a, b) 返回 a 和 b 中较大的那个
  // 如果当前净值超过了历史最高，就更新峰值；否则峰值不变
  // 注意：峰值只会单调递增，永远不会下降
  rm.peakEquity = std::max(rm.peakEquity, currentEquity);

  // ── 步骤 2：计算当前回撤 ──────────────────────────────
  // 公式：回撤 = (峰值 - 当前净值) / 峰值
  // 当 currentEquity == peakEquity 时（处于峰值），回撤 = 0
  // 当 currentEquity < peakEquity 时（处于回撤），回撤 > 0
  //
  // 特殊情况保护：如果 peakEquity == 0（理论上不应出现），
  // 避免触发除以 0 的未定义行为，直接将回撤设为 0
  if (rm.peakEquity > 0.0) {
    rm.drawdown = (rm.peakEquity - currentEquity) / rm.peakEquity;
  } else {
    rm.drawdown = 0.0;
  }

  // ── 步骤 3：更新历史最大回撤 ─────────────────────────
  // maxDrawdown 记录整个回测期间出现过的最大回撤值
  // 用 std::max 保持它只增不减
  rm.maxDrawdown = std::max(rm.maxDrawdown, rm.drawdown);

  // ── 步骤 4：检查是否触发熔断 ─────────────────────────
  // 条件：当前回撤 >= 阈值，且尚未熔断（避免重复打印提示）
  //
  // 布尔标志（Boolean Flag）作为状态机：
  //   halted 变量有两个状态：false（正常）和 true（已熔断）
  //   一旦切换到 true，本轮循环就不再切换回 false
  //   这模拟了真实熔断的"不可逆"特性——触发就锁定，直到下一轮重置
  if (rm.drawdown >= rm.limit && !rm.halted) {
    rm.halted = true; // 触发熔断，锁定状态
    std::cout << "  [!熔断触发] " << date << "  回撤达到 "
              << rm.drawdown * 100.0 << "%"
              << "，超过阈值 " << rm.limit * 100.0 << "%"
              << "，本轮禁止开新仓。" << std::endl;
  }
}
