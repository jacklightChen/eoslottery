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
├── config.hpp (全局常量)
├── lottery.hpp
├── lottery.cpp
```

### 编译方法
``` shell
eosio-cpp -abigen -I ./ -o lottery.wasm lottery.cpp
```

### action

