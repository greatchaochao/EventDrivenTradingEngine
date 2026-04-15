# report.py — 绩效可视化脚本
#
# 职责：
#   1. 从 data/signals.csv 读取净值、回撤、价格序列
#   2. 从 data/trades.csv  读取买卖成交记录
#   3. 绘制三张子图，合并保存为 data/report.png
#
# 运行方式（在项目根目录下执行）：
#   python3 scripts/report.py
#
# 依赖：pandas、matplotlib
#   安装命令：pip install pandas matplotlib

# ── import 语句 ────────────────────────────────────────────────────────────
# 相当于 C++ 的 #include，告诉 Python "我要用这些外部库"
import pandas as pd                      # pandas：表格数据处理（类似 Excel 的 Python 版）
import matplotlib.pyplot as plt          # matplotlib：绘图库，plt 是约定俗成的别名
import matplotlib.dates as mdates        # 处理 X 轴上的日期显示格式
import os                                # 读取文件路径，判断文件是否存在

# ── 第一步：读取 CSV 文件 ───────────────────────────────────────────────────
#
# pd.read_csv("文件路径") 一行代码即可把整张 CSV 表格读入内存
# 返回值是一个 DataFrame（数据框），可以理解为一张带列名的二维表
#
# parse_dates=["Date"]：让 pandas 自动把 "Date" 列解析成日期类型
#   好处：X 轴的日期刻度自动排列，不会把 "2010-01-04" 当普通字符串处理

# 脚本被调用时，工作目录应为项目根目录
# 如果路径解析失败，给出清晰提示而非神秘报错
signals_path = "data/signals.csv"
trades_path  = "data/trades.csv"

# 检查文件是否存在（os.path.exists 返回 True/False）
if not os.path.exists(signals_path):
    print(f"错误：找不到 {signals_path}")
    print("请先运行 C++ 引擎（./build/engine）生成数据文件，再执行本脚本。")
    exit(1)   # exit(1) 表示"异常退出"，供自动化脚本检测

# 读取信号日志
signals = pd.read_csv(signals_path, parse_dates=["Date"])

# trades.csv 可能为空（本轮未触发任何成交）
# 空文件用 read_csv 读取后得到一个零行的 DataFrame，后续代码正常处理
if not os.path.exists(trades_path) or os.path.getsize(trades_path) == 0:
    # 文件不存在或为空（引擎被中途停止时会出现此情况）
    # 创建一个空的 DataFrame，列名与正常文件相同，确保后续代码不报错
    print(f"警告：{trades_path} 不存在或为空，买卖标注将跳过。")
    trades = pd.DataFrame(columns=["Date", "Action", "Price",
                                   "Quantity", "Commission", "CashAfter"])
else:
    trades = pd.read_csv(trades_path, parse_dates=["Date"])

print(f"已读取 {len(signals)} 条行情记录，{len(trades)} 条成交记录。")

# ── 第二步：从 trades 中分离买单和卖单 ────────────────────────────────────
#
# DataFrame 的布尔索引（Boolean Indexing）：
#   trades["Action"] == "BUY" 返回一列 True/False
#   把这列 True/False 放进 trades[...] 就能筛选出符合条件的行
#
# 使用 str.strip() 先去掉首尾空格，避免因格式问题漏掉记录
# （C++ 引擎写入时某些 Action 值可能含尾部空格，例如 "BUY "）
if not trades.empty:
    trades["Action"] = trades["Action"].str.strip()   # 去掉首尾空格
    buys  = trades[trades["Action"] == "BUY"]         # 筛选所有买入记录
    sells = trades[trades["Action"] == "SELL"]        # 筛选所有卖出记录
else:
    buys  = trades.copy()   # 空表，后续绘图时自动跳过
    sells = trades.copy()

# ── 第三步：创建画布，排列三个子图 ────────────────────────────────────────
#
# plt.subplots(行数, 列数, figsize=(宽英寸, 高英寸))
#   返回两个值：
#     fig   — 整张画布对象（Figure），控制画布尺寸、保存等全局操作
#     axes  — 子图数组（长度 = 行数×列数），axes[0] 是第一张，axes[1] 是第二张 ...
#
# sharex=True：所有子图共享同一个 X 轴（日期轴），滚动或缩放时同步移动
fig, axes = plt.subplots(3, 1, figsize=(14, 10), sharex=True)

# 设置整张图的标题
fig.suptitle("Event-Driven Trading Engine — Performance Report", fontsize=14)

# ── 子图 ①：Equity Curve（资金净值曲线）─────────────────────────────────
#
# axes[0].plot(x轴数据, y轴数据, color=..., linewidth=...)
#   x 轴：signals["Date"]     — 日期序列
#   y 轴：signals["NetValue"] — 每日账户总净值
ax1 = axes[0]
ax1.plot(signals["Date"], signals["NetValue"],
         color="steelblue", linewidth=1.2, label="Net Value")

# 画一条初始资金（10000）的水平参考线，方便判断是盈利还是亏损
ax1.axhline(y=10000, color="gray", linestyle="--", linewidth=0.8, label="Initial Capital (10000)")

ax1.set_ylabel("Net Value (USD)")          # Y 轴标签
ax1.set_title("① Equity Curve")           # 子图标题
ax1.legend(loc="upper left", fontsize=8)  # 图例（loc 控制位置）
ax1.grid(True, alpha=0.3)                 # 背景网格，alpha 控制透明度

# ── 子图 ②：Drawdown（回撤曲线）─────────────────────────────────────────
#
# fill_between：在折线与 y=0 之间填充颜色，让回撤区域更直观
#   alpha=0.5 — 50% 透明度，避免颜色过重遮住细节
#
# Drawdown 在 CSV 中是 0.0~1.0 的小数（例如 0.05 = 5%）
# 乘以 100 转成百分比，Y 轴更易读
ax2 = axes[1]
drawdown_pct = signals["Drawdown"] * 100   # 转换为百分比

ax2.fill_between(signals["Date"], drawdown_pct, 0,
                 color="tomato", alpha=0.5, label="Drawdown")
ax2.plot(signals["Date"], drawdown_pct,
         color="tomato", linewidth=0.8)

# 画一条 10% 熔断阈值的参考线（与 risk.h 中的 limit = 0.10 对应）
ax2.axhline(y=10.0, color="darkred", linestyle="--",
            linewidth=1.0, label="Circuit Breaker (10%)")

ax2.set_ylabel("Drawdown (%)")
ax2.set_title("② Drawdown")
ax2.legend(loc="lower left", fontsize=8)
ax2.grid(True, alpha=0.3)

# ── 子图 ③：Price + Trade Markers（价格走势 + 买卖点）────────────────────
#
# 先画灰色的价格折线作为背景
# 再用 scatter（散点图）在对应日期叠加买卖标记
#   marker="^" — 向上的三角形，常用于表示"买入"
#   marker="v" — 向下的三角形，常用于表示"卖出"
#   zorder=5   — 控制绘制层级，数字越大越靠前，确保标记不被折线遮挡
ax3 = axes[2]
ax3.plot(signals["Date"], signals["Price"],
         color="gray", linewidth=0.8, label="Price")

# 只有在有成交记录时才绘制标记（避免 scatter 收到空数据报警告）
if not buys.empty:
    ax3.scatter(buys["Date"], buys["Price"],
                marker="^", color="green", s=60, zorder=5, label="BUY")

if not sells.empty:
    ax3.scatter(sells["Date"], sells["Price"],
                marker="v", color="red",   s=60, zorder=5, label="SELL")

ax3.set_ylabel("Price (USD)")
ax3.set_title("③ Price Chart with Trade Signals")
ax3.legend(loc="upper left", fontsize=8)
ax3.grid(True, alpha=0.3)

# ── 第四步：格式化 X 轴日期刻度 ──────────────────────────────────────────
#
# 由于三张子图共享 X 轴（sharex=True），只需在最后一张子图设置即可
# AutoDateLocator：自动根据数据跨度选择合适的刻度间距
# AutoDateFormatter：自动选择合适的日期显示格式（年/月/日）
ax3.xaxis.set_major_locator(mdates.AutoDateLocator())
ax3.xaxis.set_major_formatter(mdates.AutoDateFormatter(mdates.AutoDateLocator()))
fig.autofmt_xdate(rotation=45)  # 将日期标签旋转 45 度，防止重叠

# ── 第五步：调整子图间距并保存 ────────────────────────────────────────────
#
# tight_layout：自动调整子图之间的间距，防止标题/标签被裁切
# rect=[左, 下, 右, 上]：为顶部的 suptitle 留出空间（上边距缩小至 0.95）
plt.tight_layout(rect=[0, 0, 1, 0.95])

# 保存路径：data/report.png
# dpi=150：每英寸 150 像素，适合屏幕展示（dpi=300 适合打印，文件更大）
output_path = "data/report.png"
plt.savefig(output_path, dpi=150)

print(f"报告已生成：{output_path}")
print("使用文件管理器或浏览器打开该 PNG 文件即可查看图表。")
