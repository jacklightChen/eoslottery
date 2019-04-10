#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>
#include "config.hpp"

using namespace eosio;

class [[eosio::contract("lottery4test")]] lottery4test : public contract {
  public:
    using contract::contract;
    lottery4test(eosio::name receiver, eosio::name code, datastream<const char*> ds):contract(receiver, code, ds)
    ,_global(get_self(), get_first_receiver().value),
    _rounds(get_self(), get_first_receiver().value),
    _roundfee(get_self(), get_first_receiver().value)
    {
      _global_state = _global.exists() ? _global.get() : get_default_parameters();
    }
    
    ~lottery4test() {
        _global.set(_global_state, _self);
    }

    const uint64_t useconds_draw_interval = 2*3600*uint64_t(1000000);
    const uint64_t useconds_claim_token_bonus_interval = 24*3600*uint64_t(1000000);
    const uint16_t max_player_bills_per_round = 50;
    
    struct [[eosio::table]] global_item {
        asset    key_price;
        uint64_t cur_round;
        asset    total_claimed_token_bonus;
        uint32_t total_users;
        bool  active;
    };
    
    typedef eosio::singleton<"global"_n, global_item> global_singleton;
    typedef eosio::multi_index<"global"_n, global_item> dump_for_global_singleton;
    
     struct [[eosio::table]] bill_item {
        uint64_t id;
        name player;
        uint64_t low;
        uint64_t high;
        uint64_t time;
        uint64_t primary_key()const {return id;}
        name byplayer()const { return player; }
    };
    
    typedef eosio::multi_index<"bills"_n, bill_item> bills_table;
  
    global_item get_default_parameters();
    
    void newround(name actor);

    void endround();

    [[eosio::action]]
    void transfer(name from, name to, asset quantity, std::string memo);
    
    [[eosio::action]]
    bill_item buykeys(name buyer, asset quantity, std::string memo);

    uint64_t randomkey(uint64_t max);
    
    [[eosio::action]]
    void active(name actor);

    [[eosio::action]]
    void setactived(bool actived);
    
    [[eosio::action]]
    void delaydraw(name drawer);

    [[eosio::action]]
    void drawing(name drawer);

    [[eosio::action]]
    void draw(name drawer);
			      
  private:
    struct [[eosio::table]] round_item {
        uint64_t round;
        uint64_t bills_num;
        uint64_t last_key;
        asset reward_bucket;
        name draw_account;
        uint64_t start_time;
        uint64_t draw_time;
        uint64_t lucky_key;
        name lucky_player;
        uint64_t primary_key()const {return round;}
    };
    typedef eosio::multi_index<"rounds"_n, round_item> rounds_table;
    
    struct roundfee_item {
        uint64_t round;
        asset    to_lucky;
        uint64_t primary_key()const {return round;}
    };
    typedef eosio::multi_index<"roundfee"_n, roundfee_item> roundfee_table;
    
    
    global_singleton _global;
    global_item _global_state;
    rounds_table _rounds;
    roundfee_table _roundfee;
};

