# YTX

Welcome! I’d like to introduce you to YTX, a stand-alone software designed to simplify your work, inspired by [GnuCash](https://gnucash.org). It functions as a lightweight ERP system to streamline your operations. I hope YTX will be a valuable tool in helping you manage your tasks more efficiently. Dive in and start exploring — Excited to help you get the most out of your experience!

## Developer Guide

### Node, Sqlite3 And Enum

| Node            |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |  party  | employee  | date_time |  color  |  document   |    first     |   second    | discount  | finished  | initial_total | final_total |
| --------------- | :-----: | :-----: | :-----: | :----------: | :-----: | :-----: | :-----: | :-----: | :-----: | :-------: | :-------: | :-----: | :---------: | :----------: | :---------: | :-------: | :-------: | :-----------: | :---------: |
| EnumSearch      |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  | kParty  | kEmployee | kDateTime | KColor  |  kDocument  |    kFirst    |   kSecond   | kDiscount | kFinished | kInitialTotal | kFinalTotal |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| Qt              | QString |   int   | QString |   QString    | QString |  bool   |   int   |   int   |   int   |    int    |  QString  | QString | QStringList |    double    |   double    |  double   |   bool    |    double     |   double    |
| Sqlite3         |  TEXT   | INTEGER |  TEXT   |     TEXT     |  TEXT   | BOOLEAN | INTEGER | INTEGER | INTEGER |  INTEGER  |   DATE    |  TEXT   |    TEXT     |   NUMERIC    |   NUMERIC   |  NUMERIC  |  BOOLEAN  |    NUMERIC    |   NUMERIC   |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| finance         |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |    X    |     X     |     X     |    X    |      X      |      X       |      X      |     X     |     X     | foreign_total | local_total |
| TreeEnum        |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  |    X    |     X     |     X     |    X    |      X      |      X       |      X      |     X     |     X     | kForeignTotal | kLocalTotal |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| task            |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |    X    |     X     | date_time |  color  |  document   |  unit_cost   |      X      |     X     | finished  |   quantity    |   amount    |
| TreeEnumTask    |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  |    X    |     X     | kDateTime | kColor  |  kDocument  |  kUnitCost   |      X      |     X     | kFinished |   kQuantity   |   kAmount   |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| product         |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |    X    |     X     |     X     |  color  |      X      |  unit_price  | commission  |     X     |     X     |   quantity    |   amount    |
| EnumProduct     |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  |    X    |     X     |     X     | kColor  |      X      |  kUnitPrice  | kCommission |     X     |     X     |   kQuantity   |   kAmount   |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| stakeholder     |  name   |   id    |  code   | description  |  note   |  rule   |  type   |  unit   |    X    | employee  | deadline  |    X    |      X      | payment_term |  tax_rate   |     X     |     X     |    amount     |      X      |
| EnumStakeholder |  kName  |   kID   |  kCode  | kDescription |  kNote  |  kRule  |  kType  |  kUnit  |    X    | kEmployee | kDeadline |    X    |      X      | kPaymentTerm |  kTaxRate   |     X     |     X     |    kAmount    |      X      |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| purchase        |  name   |   id    |    X    | description  |    X    |  rule   |  type   |  unit   |  party  | employee  | date_time |    X    |      X      |    first     |   second    | discount  | finished  | gross_amount  | net_amount  |
| EnumOrder       |  kName  |   kID   |    X    | kDescription |    X    |  kRule  |  kType  |  kUnit  | kParty  | kEmployee | kDateTime |    X    |      X      |    kFirst    |   kSecond   | kDiscount | kFinished | kGrossAmount  | kNetAmount  |
|                 |         |         |         |              |         |         |         |         |         |           |           |         |             |              |             |           |           |               |             |
| sales           |  name   |   id    |    X    | description  |    X    |  rule   |  type   |  unit   |  party  | employee  | date_time |    X    |      X      |    first     |   second    | discount  | finished  | gross_amount  | net_amount  |
| EnumOrder       |  kName  |   kID   |    X    | kDescription |    X    |  kRule  |  kType  |  kUnit  | kParty  | kEmployee | kDateTime |    X    |      X      |    kFirst    |   kSecond   | kDiscount | kFinished | kGrossAmount  | kNetAmount  |

### Trans, Sqlite3 And Enum

| Trans           |   id    | date_time |  code   | lhs_node | lhs_ratio | lhs_debit | lhs_credit | description  | unit_price |   support_id    | discount_price | net_amount |  document   |  state  |  rhs_credit  | rhs_debit | rhs_ratio |    rhs_node    |
| --------------- | :-----: | :-------: | :-----: | :------: | :-------: | :-------: | :--------: | :----------: | :--------: | :-------------: | :------------: | :--------: | :---------: | :-----: | :----------: | :-------: | :-------: | :------------: |
| EnumSearch      |   kID   | kDateTime |  KCode  | kLhsNode | kLhsRatio | kLhsDebit | kLhsCredit | kDescription | kUnitPrice |   kSupportID    | kDiscountPrice | kNetAmount |  kDocument  | kState  |  kRhsCredit  | kRhsDebit | kRhsRatio |    kRhsNode    |
|                 |         |           |         |          |           |           |            |              |            |                 |                |            |             |         |              |           |           |                |
| Qt              |   int   |  QString  | QString |   int    |  double   |  double   |   double   |   QString    |   double   |       int       |     double     |   double   | QStringList |  bool   |    double    |  double   |  double   |      int       |
| Sqlite3         | INTEGER |   TEXT    |  TEXT   | INTEGER  |  NUMERIC  |  NUMERIC  |  NUMERIC   |     TEXT     |  NUMERIC   |    INTERGER     |    NUMERIC     |  NUMERIC   |    TEXT     | BOOLEAN |   NUMERIC    |  NUMERIC  |  NUMERIC  |    INTEGER     |
|                 |         |           |         |          |           |           |            |              |            |                 |                |            |             |         |              |           |           |                |
| finance         |   id    | date_time |  code   | lhs_node | lhs_ratio | lhs_debit | lhs_credit | description  |     X      |   support_id    |       X        |     X      |  document   |  state  |  rhs_credit  | rhs_debit | rhs_ratio |    rhs_node    |
| TableEnum       |   kID   | kDateTime |  kCode  |    X     | kLhsRatio |  kDebit   |  kCredit   | kDescription |     X      |   kSupportID    |       X        |     X      |  kDocument  | kState  |      X       |     X     |     X     |    kRhsNode    |
|                 |         |           |         |          |           |           |            |              |            |                 |                |            |             |         |              |           |           |                |
| task            |   id    | date_time |  code   | lhs_node |     X     | lhs_debit | lhs_credit | description  | unit_cost  |   support_id    |       X        |     X      |  document   |  state  |  rhs_credit  | rhs_debit |     X     |    rhs_node    |
| TableEnum       |   kID   | kDateTime |  kCode  |    X     |     X     |  kDebit   |  kCredit   | kDescription | kUnitCost  |   kSupportID    |       X        |     X      |  kDocument  | kState  |      X       |     X     |     X     |    kRhsNode    |
|                 |         |           |         |          |           |           |            |              |            |                 |                |            |             |         |              |           |           |                |
| product         |   id    | date_time |  code   | lhs_node |     X     | lhs_debit | lhs_credit | description  | unit_cost  |        X        |       X        |     X      |  document   |  state  |  rhs_credit  | rhs_debit |     X     |    rhs_node    |
| TableEnum       |   kID   | kDateTime |  kCode  |    X     |     X     |  kDebit   |  kCredit   | kDescription | kUnitCost  |        X        |       X        |     X      |  kDocument  | kState  |      X       |     X     |     X     |    kRhsNode    |
|                 |         |           |         |          |           |           |            |              |            |                 |                |            |             |         |              |           |           |                |
| stakeholder     |   id    | date_time |  code   | lhs_node |     X     |     X     |     X      | description  | unit_price | outside_product |       X        |     X      |  document   |  state  |      X       |     X     |     X     | inside_product |
| EnumStakeholder |   kID   | kDateTime |  kCode  |    X     |     X     |     X     |     X      | kDescription | kUnitPrice | kOutsideProduct |       X        |     X      |  kDocument  | kState  |      X       |     X     |     X     | kInsideProduct |
|                 |         |           |         |          |           |           |            |              |            |                 |                |            |             |         |              |           |           |                |
| purchase        |   id    |     X     |  code   | lhs_node |     X     |   first   |   second   | description  | unit_price | outside_product | discount_price | net_amount |      X      |    X    | gross_amount | discount  |     X     | inside_product |
| EnumOrder       |   kID   |     X     |  kCode  |    X     |     X     |  kFirst   |  kSecond   | kDescription | kUnitPrice | kOutsideProduct | kDiscountPrice | kNetAmount |      X      |    X    | kGrossAmount | kDiscount |     X     | kInsideProduct |
|                 |         |           |         |          |           |           |            |              |            |                 |                |            |             |         |              |           |           |                |
| sales           |   id    |     X     |  code   | lhs_node |     X     |   first   |   second   | description  | unit_price | outside_product | discount_price | net_amount |      X      |    X    | gross_amount | discount  |     X     | inside_product |
| EnumOrder       |   kID   |     X     |  kCode  |    X     |     X     |  kFirst   |  kSecond   | kDescription | kUnitPrice | kOutsideProduct | kDiscountPrice | kNetAmount |      X      |    X    | kGrossAmount | kDiscount |     X     | kInsideProduct |

### Build Requirements

This is a pure Qt project. Only a compatible version of Qt and a compiler are required.

- Qt: 6.5+
    1. Desktop
    2. Additional Libraries
        - Qt Charts
        - Qt Image Formats
        - Developer and Designer Tools
    3. Qt Creator 15.X.X
- CMake: 3.19+
- Compiler: GCC 12+ or LLVM/Clang 14+

## User Guide

### Introduction

1. Most features are unavailable until the database is opened.
2. Files
    - New: **`Ctrl + Alt+ N`**
    - Open
        1. Drag and drop
        2. Double-click
        3. Open from recent files
        4. Shortcut: **`Ctrl + O`**
3. File Lock: A lock file with a .lock extension is created alongside your database file to prevent it from being opened by multiple instances simultaneously, ensuring data integrity.
4. Configuration Directory
    - QStandardPaths::AppConfigLocation
    - Mac: `~/Library/Preferences/<APPNAME>` (Show hidden folders: **`cmd + shift + .`**)
    - Win: `C:/Users/<USER>/AppData/Local/<APPNAME>`
    - Linux: `~/.config/<APPNAME>`

### Actions

1. Preferences
    - Default Unit: Set the default unit.
2. Node
    - Insert: **`Alt + N`**, Append: **`Alt + P`**
    - Rules(**R**)
        1. Define how balances are calculated in the transaction table. New nodes inherit rules from their parent nodes by default.
        2. When properly configured, the total of all nodes is typically positive.
        3. Two common rules:
            - **DICD**: _Debit Increase, Credit Decrease_. Used for assets and expenses, calculated as "left side minus right side".
            - **DDCI**: _Debit Decrease, Credit Increase_. Used for liabilities, income, and owner’s equity, calculated as "right side minus left side".
    - Type
        1. **B**: Branch nodes (grouping nodes; cannot record transactions).
        2. **L**: Leaf nodes (used to record transactions).
        3. **S**: Support nodes (easy viewing; no transactions; supports specific actions).
    - Unit(**U**)
        - Finance/Product/Task/Order node inherits its parent node’s unit by default.
        - Stakeholder node inherits default unit.
3. Transaction
    - Append: **`Ctrl + N`**
    - Reference date and related node.
    - Date
        1. By default, the time is displayed and stored in the database with second-level precision.
        2. Shortcut keys (for English input method):
            - T: Now
            - J: Yesterday
            - K: Tomorrow
            - H: End of last month
            - L: First of next month
            - W: Last week
            - B: Next week
            - E: Last year
            - N: next year
    - Ratio: Represents the conversion rate between the node’s unit and the base unit (e.g., 1 USD = 7.2 RMB).
    - Document(**D**)
        1. No restrictions on type and quantity.
        2. Local only, but files can be backed up via cloud services like Dropbox or Google Drive.
    - Status(**S**)
        1. Used for reconciliation (sort by date, then reconcile).
    - Related Node
        1. If no related node is specified, the row will not be saved when the table is closed.
    - Debit, Credit, Balance
        1. Display values in the node’s current unit, where the base unit value = ratio × node value.
4. Status bar
    - The middle section shows the node’s total value in the current unit.
    - The right section shows the result of operations between two nodes.
