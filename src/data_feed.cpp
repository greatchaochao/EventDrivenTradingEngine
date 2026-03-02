#include "data_feed.h"
#include <fstream>
#include <iostream>
#include <sstream>

// 关于参数 `const std::string &filename` 的解释：
//
// ── 普通传值（std::string filename）──────────────────────────────
//   调用函数时，C++ 会把整个字符串「复制一份」再传进来。
//   如果字符串很长，这次复制会浪费时间和内存。
//
// ── 引用（std::string &filename）────────────────────────────────
//   & 表示「引用」，即传入的是原变量的「别名」，不复制。
//   函数内部直接操作外部的那块内存 —— 速度更快，零复制开销。
//   但有风险：函数内部可以直接修改外部变量的内容。
//
// ── 常量引用（const std::string &filename）──────────────────────
//   const + & 是最常见的组合：
//     - & ：不复制，直接使用外部内存（高效）
//     - const：承诺「只读」，函数内部不允许修改这个字符串
//   这样既获得了引用的速度优势，又保证了调用者的数据安全。
//
// 口诀：「传大对象用 const &，又快又安全。」
std::vector<Tick> loadCSV(const std::string &filename) {
  std::vector<Tick> ticks;      // 用于存储所有有效行情数据的列表
  std::ifstream file(filename); // 以只读方式打开文件

  // 检查文件是否成功打开；若失败则打印错误并返回空列表
  if (!file.is_open()) {
    std::cerr << "错误：无法打开文件 " << filename << std::endl;
    return ticks;
  }

  std::string line;
  std::getline(file, line); // 跳过第一行表头（Date, AAPL.O, MSFT.O, ...）

  // 逐行读取文件，每次循环处理一条行情记录
  while (std::getline(file, line)) {
    std::stringstream ss(line); // 将该行文本包装成流，方便按逗号分割
    std::string date, priceStr;

    std::getline(ss, date, ',');     // 第 0 列：交易日期
    std::getline(ss, priceStr, ','); // 第 1 列：AAPL.O 收盘价（字符串形式）

    // 若价格字段为空（说明该日无苹果行情），跳过本行
    if (priceStr.empty())
      continue;

    Tick t;
    t.date = date;
    t.price = std::stod(priceStr); // 将字符串转换为 double 类型的数字
    ticks.push_back(t);            // 将本条记录追加到列表末尾
  }

  return ticks; // 返回包含所有有效行情的列表
}