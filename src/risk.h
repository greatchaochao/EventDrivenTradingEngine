#pragma once
// risk.h — 风控模块的声明文件
// 本文件定义了风险管理器所需的数据类型，并声明了两个函数
// 任何需要风控功能的文件，只需 #include "risk.h" 即可

#include <string>

// ─────────────────────────────────────────────
// RiskManager：实时记录账户的风险状态
//
//   peakEquity  — 账户历史最高净值（运行中只会增大，永不减小）
//   drawdown    — 当前回撤比例（0.0 ~ 1.0，例如 0.10 表示 10% 回撤）
//   maxDrawdown — 历史最大回撤（记录整个回测期间出现过的最大回撤）
//   halted      — 熔断标志：false = 正常运行；true = 已触发熔断，禁止开新仓
//   limit       — 触发熔断的回撤阈值（例如 0.10 表示亏损超过 10% 就熔断）
//
// 设计原则：
//   RiskManager 是一个纯数据容器，不做任何输出
//   所有更新逻辑都在下方的 updateRisk() 函数中完成
// ─────────────────────────────────────────────
struct RiskManager {
  double peakEquity;  // 历史最高净值
  double drawdown;    // 当前回撤（本 tick 的实时值）
  double maxDrawdown; // 历史最大回撤（整个周期内的峰值）
  bool halted;        // 熔断开关（一旦为 true，本轮禁止买入）
  double limit;       // 回撤阈值（超过此值则触发熔断）
};

// ─────────────────────────────────────────────
// createRiskManager：初始化风险管理器
//
// 参数：
//   initialEquity — 初始账户净值（与 Portfolio 的初始资金保持一致）
//   limit         — 熔断回撤阈值，默认 0.10（即 10%）
//
// 返回：一个"峰值 = 初始资金、未熔断"的 RiskManager 对象
// ─────────────────────────────────────────────
RiskManager createRiskManager(double initialEquity, double limit = 0.10);

// ─────────────────────────────────────────────
// updateRisk：在每个 tick 更新风险状态
//
// 参数：
//   rm            — RiskManager 的引用（函数会修改它，所以用 & 而非 const &）
//   currentEquity — 本 tick 的账户总净值（来自 getNetValue）
//   date          — 本 tick 的日期字符串（仅用于打印提示信息）
//
// 函数内部逻辑（按顺序执行）：
//   1. 用 std::max 更新 peakEquity（峰值只升不降）
//   2. 计算 drawdown = (peak - current) / peak
//   3. 用 std::max 更新 maxDrawdown
//   4. 如果 drawdown >= limit 且尚未熔断，将 halted 设为 true 并打印提示
//
// 返回：void（无返回值，所有修改通过引用直接作用于调用方的变量）
// ─────────────────────────────────────────────
void updateRisk(RiskManager &rm, double currentEquity, const std::string &date);
