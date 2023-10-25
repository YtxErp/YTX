# YTX

## 开发者

### 节点、数据库和枚举

| Node            |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |  party  | employee  | date_time |  color  |  document   |    first     |   second    | discount  | finished  | initial_total | final_total |
| --------------- | :-----: | :-----: | :-----: | :----------: | :-----: | :-----: | :-----: | :-----: | :-----: | :-------: | :-------: | :-----: | :---------: | :----------: | :---------: | :-------: | :-------: | :-----------: | :---------: |
| EnumSearch      |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  | kParty  | kEmployee | kDateTime | KColor  |  kDocument  |    kFirst    |   kSecond   | kDiscount | kFinished | kInitialTotal | kFinalTotal |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| Qt              | QString |   int   | QString |   QString    | QString |  bool   |   int   |   int   |   int   |    int    |  QString  | QString | QStringList |    double    |   double    |  double   |   bool    |    double     |   double    |
| Sqlite3         |  TEXT   | INTEGER |  TEXT   |     TEXT     |  TEXT   | BOOLEAN | INTEGER | INTEGER | INTEGER |  INTEGER  |   DATE    |  TEXT   |    TEXT     |   NUMERIC    |   NUMERIC   |  NUMERIC  |  BOOLEAN  |    NUMERIC    |   NUMERIC   |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| finance         |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |    X    |     X     |     X     |    X    |      X      |      X       |      X      |     X     |     X     | initial_total | final_total |
| TreeEnum        |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  |    X    |     X     |     X     |    X    |      X      |      X       |      X      |     X     |     X     | kInitialTotal | kFinalTotal |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| task            |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |    X    |     X     | date_time |  color  |  document   |  unit_cost   |      X      |     X     | finished  | initial_total | final_total |
| TreeEnumTask    |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  |    X    |     X     | kDateTime | kColor  |  kDocument  |  kUnitCost   |      X      |     X     | kFinished | kInitialTotal | kFinalTotal |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| product         |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |    X    |     X     |     X     |  color  |      X      |  unit_price  | commission  |     X     |     X     | initial_total | final_total |
| EnumProduct     |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  |    X    |     X     |     X     | kColor  |      X      |  kUnitPrice  | kCommission |     X     |     X     | kInitialTotal | kFinalTotal |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| stakeholder     |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |    X    | employee  | deadline  |    X    |      X      | payment_term |  tax_rate   |     X     |     X     |       X       |      X      |
| EnumStakeholder |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  |    X    | kEmployee | kDeadline |    X    |      X      | kPaymentTerm |  kTaxRate   |     X     |     X     |       X       |      X      |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| purchase        |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |  party  | employee  | date_time |    X    |      X      |    first     |   second    | discount  | finished  |    amount     |   settled   |
| EnumOrder       |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  | kParty  | kEmployee | kDateTime |    X    |      X      |    kFirst    |   kSecond   | kDiscount | kFinished |    kAmount    |  kSettled   |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| sales           |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |  party  | employee  | date_time |    X    |      X      |    first     |   second    | discount  | finished  |    amount     |   settled   |
| EnumOrder       |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  | kParty  | kEmployee | kDateTime |    X    |      X      |    kFirst    |   kSecond   | kDiscount | kFinished |    kAmount    |  kSettled   |

### 交易、数据库和枚举

| Trans           |   id    | date_time |  code   | lhs_node | lhs_ratio | lhs_debit | lhs_credit | description  | unit_price |   support_id    | discount_price | settled  |  document   |  state  | rhs_credit | rhs_debit | rhs_ratio |    rhs_node    |
| --------------- | :-----: | :-------: | :-----: | :------: | :-------: | :-------: | :--------: | :----------: | :--------: | :-------------: | :------------: | :------: | :---------: | :-----: | :--------: | :-------: | :-------: | :------------: |
| EnumSearch      |   kID   | kDateTime |  KCode  | kLhsNode | kLhsRatio | kLhsDebit | kLhsCredit | kDescription | kUnitPrice |   kSupportID    | kDiscountPrice | kSettled |  kDocument  | kState  | kRhsCredit | kRhsDebit | kRhsRatio |    kRhsNode    |
|                 |         |           |         |          |           |           |            |              |            |                 |                |          |             |         |            |           |           |                |
| Qt              |   int   |  QString  | QString |   int    |  double   |  double   |   double   |   QString    |   double   |       int       |     double     |  double  | QStringList |  bool   |   double   |  double   |  double   |      int       |
| Sqlite3         | INTEGER |   TEXT    |  TEXT   | INTEGER  |  NUMERIC  |  NUMERIC  |  NUMERIC   |     TEXT     |  NUMERIC   |    INTERGER     |    NUMERIC     | NUMERIC  |    TEXT     | BOOLEAN |  NUMERIC   |  NUMERIC  |  NUMERIC  |    INTEGER     |
|                 |         |           |         |          |           |           |            |              |            |                 |                |          |             |         |            |           |           |                |
| finance         |   id    | date_time |  code   | lhs_node | lhs_ratio | lhs_debit | lhs_credit | description  |     X      |   support_id    |       X        |    X     |  document   |  state  | rhs_credit | rhs_debit | rhs_ratio |    rhs_node    |
| TableEnum       |   kID   | kDateTime |  kCode  |    X     | kLhsRatio |  kDebit   |  kCredit   | kDescription |     X      |   kSupportID    |       X        |    X     |  kDocument  | kState  |     X      |     X     |     X     |    kRhsNode    |
|                 |         |           |         |          |           |           |            |              |            |                 |                |          |             |         |            |           |           |                |
| task            |   id    | date_time |  code   | lhs_node |     X     | lhs_debit | lhs_credit | description  | unit_cost  |   support_id    |       X        |    X     |  document   |  state  | rhs_credit | rhs_debit |     X     |    rhs_node    |
| TableEnum       |   kID   | kDateTime |  kCode  |    X     |     X     |  kDebit   |  kCredit   | kDescription | kUnitCost  |   kSupportID    |       X        |    X     |  kDocument  | kState  |     X      |     X     |     X     |    kRhsNode    |
|                 |         |           |         |          |           |           |            |              |            |                 |                |          |             |         |            |           |           |                |
| product         |   id    | date_time |  code   | lhs_node |     X     | lhs_debit | lhs_credit | description  | unit_cost  |        X        |       X        |    X     |  document   |  state  | rhs_credit | rhs_debit |     X     |    rhs_node    |
| TableEnum       |   kID   | kDateTime |  kCode  |    X     |     X     |  kDebit   |  kCredit   | kDescription | kUnitCost  |        X        |       X        |    X     |  kDocument  | kState  |     X      |     X     |     X     |    kRhsNode    |
|                 |         |           |         |          |           |           |            |              |            |                 |                |          |             |         |            |           |           |                |
| stakeholder     |   id    | date_time |  code   | lhs_node |     X     |     X     |     X      | description  | unit_price | outside_product |       X        |    X     |  document   |  state  |     X      |     X     |     X     | inside_product |
| EnumStakeholder |   kID   | kDateTime |  kCode  |    X     |     X     |     X     |     X      | kDescription | kUnitPrice | kOutsideProduct |       X        |    X     |  kDocument  | kState  |     X      |     X     |     X     | kInsideProduct |
|                 |         |           |         |          |           |           |            |              |            |                 |                |          |             |         |            |           |           |                |
| purchase        |   id    |     X     |  code   | lhs_node |     X     |   first   |   second   | description  | unit_price | outside_product | discount_price | settled  |      X      |    X    |   amount   | discount  |     X     | inside_product |
| EnumOrder       |   kID   |     X     |  kCode  |    X     |     X     |  kFirst   |  kSecond   | kDescription | kUnitPrice | kOutsideProduct | kDiscountPrice | kSettled |      X      |    X    |  kAmount   | kDiscount |     X     | kInsideProduct |
|                 |         |           |         |          |           |           |            |              |            |                 |                |          |             |         |            |           |           |                |
| sales           |   id    |     X     |  code   | lhs_node |     X     |   first   |   second   | description  | unit_price | outside_product | discount_price | settled  |      X      |    X    |   amount   | discount  |     X     | inside_product |
| EnumOrder       |   kID   |     X     |  kCode  |    X     |     X     |  kFirst   |  kSecond   | kDescription | kUnitPrice | kOutsideProduct | kDiscountPrice | kSettled |      X      |    X    |  kAmount   | kDiscount |     X     | kInsideProduct |

### 构建

这是一个纯 Qt 项目，只需要安装 Qt（部分组件）、CMake、和一个适应的 C++ 编译器就好。

- Qt: 6.5+
    1. Desktop
    2. Additional Libraries
        - Qt Charts
        - Qt Image Formats
        - Developer and Designer Tools
    3. Qt Creator 15.X.X
- CMake: 3.19+
- Compiler: GCC 12+ or LLVM/Clang 14+

## 用户引导

欢迎！今天向您介绍 YTX，一款旨在简化工作的单机软件；它的灵感来自 [GnuCash](https://gnucash.org)，类似于一个轻量级的ERP，现在开始吧。

### 介绍

1. 在打开数据库之前，大多数功能无法使用；因为有一些配置是放在数据库里的。
2. 文件
    - 新建: **`Ctrl + Alt+ N`**
    - 打开
        1. 拖动
        2. 双击
        3. 最近记录
        4. 快捷键: **`Ctrl + O`**
3. 文件锁: 为了确保数据完整性，会在您的数据库文件旁创建一个带有 .lock 扩展名的锁文件，以防止数据库被多个实例同时打开。
4. 配置目录
    - QStandardPaths::AppConfigLocation
    - Mac: `~/Library/Preferences/<APPNAME>` (显示隐藏文件夹: **`cmd + shift + .`**)
    - Win: `C:/Users/<USER>/AppData/Local/<APPNAME>`
    - Linux: `~/.config/<APPNAME>`

### 操作

1. 首先项
    - 默认单位: 设置新建节点时的默认单位。
2. 节点
    - 插入: **`Alt + N`**, 追加: **`Alt + P`**
    - 规则(**R**)
        1. 定义如何在交易表中计算余额；新节点默认继承其父节点的规则。
        2. 在正确配置的情况下，所有节点的总和趋向于正数。
        3. 二种规则
            - **DICD**: 借方增加，贷方减少；用于资产和费用，计算方式为“左侧减右侧”。
            - **DDCI**: 借方减少，贷方增加；用于负债、收入和所有者权益，计算方式为“右侧减左侧”。
    - 类型
        1. **B**: 枝节点（分组节点；无法记录交易）。
        2. **L**: 叶节点（用于记录交易）。
        3. **S**: 辅助节点（便于查看；无法记录交易；支持简单的几个操作）
    - 单位(**U**)
        - 财务/产品/任务/订单节点默认继承其父节点的单位。
        - 人脉节点是默认单位
3. 交易
    - 追加: **`Ctrl + N`**
    - 可以引用日期和关联节点。
    - 日期
        1. 默认情况下，时间以秒级精度显示并在数据库中存储
        2. 快捷键 (英文输入法):
            - T: 现在
            - J: 昨天
            - K: 明天
            - H: 上月底
            - L: 下月初
            - W: 上周
            - B: 下周
            - E: 去年
            - N: 明年
    - 比率: 表示节点单位与基准单位之间的换算率（例如，1 USD = 7.2 RMB）
    - 文档(**D**)
        1. 不限制类型和数量。
        2. 仅限本地，但文档可以通过 Dropbox 或 Google Drive 等云服务备份。
    - 状态(**S**)
        1. 用于对账（比如按日期排序后对账）。
    - 关联节点
        1. 如果未指定关联节点，则在关闭页面时该行将不会被保存。
    - 借方、贷方、余额
        1. 以节点当前单位显示值，其中基准单位值 = 比率 × 节点值。
4. 状态栏
    - 中间部分显示节点在当前单位下的总值。
    - 右侧部分显示两个节点之间操作的结果。
