#include "portfolio.h"
#include <cmath>    // std::floor — 向下取整，用于计算可买股数
#include <iostream> // std::cout — 打印成交提示

// ─────────────────────────────────────────────
// COMMISSION_RATE：手续费率常量
//
// constexpr — "compile-time constant"（编译期常量）
//   程序编译时编译器直接把 0.001 替换进去，运行时不占用任何内存
//   比 #define COMMISSION_RATE 0.001 更安全，因为它有类型（double）
//
// static — 限制此常量只在本文件内可见
//   其他文件无法访问 COMMISSION_RATE，防止命名冲突
// ─────────────────────────────────────────────
static constexpr double COMMISSION_RATE = 0.001; // 0.1% 手续费率

// ─────────────────────────────────────────────
// createPortfolio 实现
// ─────────────────────────────────────────────
Portfolio createPortfolio(double initialCash) {
  Portfolio p;
  p.cash = initialCash; // 初始现金
  p.position = 0;       // 初始持仓为零
  // p.tradeLog 是 std::vector，默认构造为空列表，无需手动初始化
  return p;
}

// ─────────────────────────────────────────────
// executeBuy 实现
// ─────────────────────────────────────────────
bool executeBuy(Portfolio &p, const std::string &date, double price) {

  // ── 检查 1：是否已有持仓 ──────────────────────────────
  // 本系统采用"满仓/空仓"模式（all-in / all-out）：
  //   有持仓时不重复买入，避免资金管理复杂化
  if (p.position > 0) {
    return false; // 已有持仓，忽略本次 BUY 信号
  }

  // ── 计算可买股数 ──────────────────────────────────────
  // 设可买股数为 n，需满足：n × price × (1 + rate) ≤ cash
  // 解方程得：n ≤ cash / (price × (1 + rate))
  // 因为股数必须是整数，对结果向下取整（不能买半股）
  //
  // std::floor(x)：返回不超过 x 的最大整数
  //   std::floor(3.9) == 3.0
  //   std::floor(3.1) == 3.0
  //
  // static_cast<int>(...)：将 double 转换为 int
  //   必须显式转换，否则编译器会报"精度损失"警告
  int shares =
      static_cast<int>(std::floor(p.cash / (price * (1.0 + COMMISSION_RATE))));

  // ── 检查 2：现金是否足够买入至少 1 股 ───────────────────
  if (shares <= 0) {
    return false; // 现金不足，无法成交
  }

  // ── 计算本次成交的各项金额 ────────────────────────────
  double commission = shares * price * COMMISSION_RATE; // 手续费
  double totalCost = shares * price + commission;       // 总花费

  // ── 更新账户状态 ──────────────────────────────────────
  p.cash -= totalCost; // 扣除现金
  p.position = shares; // 增加持仓

  // ── 记录本次成交 ──────────────────────────────────────
  Trade t;
  t.date = date;
  t.action = "BUY";
  t.price = price;
  t.quantity = shares;
  t.commission = commission;
  t.cashAfter = p.cash;

  // push_back：把 t 追加到 tradeLog 列表末尾
  p.tradeLog.push_back(t);

  return true; // 成交成功
}

// ─────────────────────────────────────────────
// executeSell 实现
// ─────────────────────────────────────────────
bool executeSell(Portfolio &p, const std::string &date, double price) {

  // ── 检查：是否有持仓可以卖出 ─────────────────────────
  if (p.position == 0) {
    return false; // 空仓，忽略本次 SELL 信号
  }

  int shares = p.position; // 卖出全部持仓

  // ── 计算本次成交的各项金额 ────────────────────────────
  double commission = shares * price * COMMISSION_RATE; // 手续费
  double proceeds = shares * price - commission;        // 卖出净收入

  // ── 更新账户状态 ──────────────────────────────────────
  p.cash += proceeds; // 收回现金
  p.position = 0;     // 清空持仓

  // ── 记录本次成交 ──────────────────────────────────────
  Trade t;
  t.date = date;
  t.action = "SELL";
  t.price = price;
  t.quantity = shares;
  t.commission = commission;
  t.cashAfter = p.cash;

  p.tradeLog.push_back(t);

  return true; // 成交成功
}

// ─────────────────────────────────────────────
// getNetValue 实现
// ─────────────────────────────────────────────
double getNetValue(const Portfolio &p, double currentPrice) {
  // 账户总价值 = 可用现金 + 持仓股票的实时市值
  // 如果 position == 0（空仓），持仓市值为 0，总价值即现金
  return p.cash + p.position * currentPrice;
}
