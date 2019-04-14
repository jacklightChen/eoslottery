# eoslottery
![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)
### 简介
基于EOS的福利彩票<br>
预计实现每小时自动开奖并分发奖励

### 使用技术
智能合约 | ... 
:---:|:---
EOS | [eosio v1.7.1](https://github.com/EOSIO/eos)
编译工具 | [eosio.cdt v1.6.1](https://github.com/EOSIO/eosio.cdt)
测试网 | CryptoKylin Testnet RPC 入口:https://api-kylin.eoslaomao.com

### 目录结构
```
.
├── lottery.hpp
├── lottery.cpp
```

### 编译方法
``` shell
eosio-cpp -abigen -I ./ -o lottery.wasm lottery.cpp
```

### action
eosio.token transfer
active
delaydraw
### eos问题汇总
https://eosio.stackexchange.com

### 一些常见问题
1.provided permissions [{"actor":"lottery1test","permission":"eosio.code"}]
```shell
合约账户缺少eosio.token的权限
解决方法:
cleos -u https://api-kylin.eoslaomao.com set account permission YOUR_ACCOUNT_NAME active '{"threshold": 1, "keys":[{"key":"YOUR_PUBLIC_KEY", "weight":1}], "accounts":[{"permission":{"actor":"YOUR_ACCOUNT_NAME","permission":"eosio.code"},"weight":1}], "waits":[] }' owner -p YOUR_ACCOUNT_NAME
```

2.asset 运算
```shell
An eosio::asset::amount is of type int64_t, therefore when you divide, any decimal places will be truncated.
如需对asset类进行运算请使用以下的形式防止一些意外的类型转换损失精度
eosio::asset y(x.amount/10, eosio::symbol("EOS",4)); // Set asset y to have 10th of x
```
