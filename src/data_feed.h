#pragma once
#include <string>
#include <vector>

// Tick：一条苹果公司（AAPL.O）的市场行情数据，对应 CSV 文件中的一行
//
// struct（结构体）：将多个相关变量打包成一个自定义类型
//
//   没有 struct 时，需要用分散的变量来表示一条行情：
//     std::string date1, date2, date3...
//     double price1, price2, price3...
//   变量一多就很难管理，而且 date1 和 price2 之间没有任何关联关系
//
//   有了 struct，一条行情就是一个 Tick 对象：
//     Tick t;
//     t.date  = "2010-01-04";
//     t.price = 30.57;
//   date 和 price 被绑定在同一个对象里，清晰且不会混淆
//
//   struct 定义的是"模板"，不是数据本身
//   就像设计图纸：图纸本身不是房子，但可以用它建造很多栋一样结构的房子
//   每次写 Tick t; 就是按照这张图纸建造一个新的 Tick 对象
struct Tick {
  std::string date; // 交易日期，例如 "2010-01-04"
  double price;     // 当日收盘价
};

// loadCSV：读取 tr_eikon_eod_data.csv，返回所有有效的 AAPL.O 行情数据
// 若某行的苹果价格字段为空，则自动跳过该行
std::vector<Tick> loadCSV(const std::string &filename);
