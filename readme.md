# eoslottery
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

### eos问题汇总
https://eosio.stackexchange.com

### 一些常见问题

provided permissions [{"actor":"lottery1test","permission":"eosio.code"}]

asset 运算
An eosio::asset::amount is of type int64_t, therefore when you divide, any decimal places will be truncated. For example:

int x = 15 / 10; // *should* equal 1.5
eosio::print(x); // Output will be 1, because the .5 is rounded down because x is an integer
Therefore, you must be aware that doing division with eosio::asset will cause some loss of precision. However, to do division with eosio::asset, you would do so as follows:

eosio::asset x(15000, eosio::symbol("EOS",4)); // Create 1.5 EOS
eosio::asset y(x.amount/10, eosio::symbol("EOS",4)); // Set asset y to have 10th of x


