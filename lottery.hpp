#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>

#define TOKEN_CONTRACT "eosio.token"_n
#define EOS_SYMBOL symbol("EOS", 4)
#define FIRST_PRIZE_SHARE 0.5f
#define SECOND_PRIZE_SHARE 0.2f
#define THIRD_PRIZE_SHARE 0.1f
#define FOURTH_PRIZE_SHARE 0.05f

using namespace eosio;

class [[eosio::contract]] lottery : public contract {
  public:
    using contract::contract;
    lottery(eosio::name receiver, eosio::name code, datastream<const char*> ds):contract(receiver, code, ds)
    ,_global(receiver, receiver.value),
    _rounds(receiver, receiver.value),
    _results(receiver, receiver.value)
    {
      _global_state = _global.exists() ? _global.get() : get_default_parameters();
    }
    
    ~lottery() {
        _global.set(_global_state, _self);
    }
    
    //1 hour
    const uint64_t useconds_draw_interval = uint64_t(3600);
    
    struct [[eosio::table]] global_item {
        asset    key_price;
        uint64_t cur_round;
        uint64_t total_users;
        bool  active;
    };
    
    typedef eosio::singleton<"global"_n, global_item> global_singleton;
    typedef eosio::multi_index<"global"_n, global_item> dump_for_global_singleton;
    
     struct [[eosio::table]] bill_item {
        uint64_t bill_id;
        name player;
        std::string data;
        uint64_t time;
        uint64_t primary_key()const {return bill_id;}
        name byplayer()const { return player; }
    };
    
    typedef eosio::multi_index<"bills"_n, bill_item> bills_table;

    [[eosio::action]]
    void active(name actor);

    [[eosio::action]]
    void setactived(bool actived);
    
    [[eosio::action]]
    void delaydraw();
    
    [[eosio::action]]
    void drawing(name drawer);
    
    [[eosio::action]]
    void draw(name drawer);
		
		global_item get_default_parameters();
    
    void newround(name actor);

    void endround();

    void transfer(name from, name to, asset quantity, std::string memo);
    
    void buykeys(name buyer, asset quantity, std::string memo);

    checksum256 randomkey();
    
		std::vector<std::string> spilt(const std::string &str, const std::string &delim);
		
		std::string keytostring(checksum256 randomkey);
		 [[eosio::action]]
		void erase(){
  auto res = _results.find(0);
  _results.erase(res);
  res = _results.find(1);
  _results.erase(res);
   res = _results.find(2);
  _results.erase(res);
  _global_state.cur_round=1;
  
}
  private:
    struct [[eosio::table]] round_item {
        uint64_t round;
        uint64_t bills_num;
        asset reward_bucket;
        uint64_t start_time;
        uint64_t draw_time;
        checksum256 lucky_key;
        std::string lucky_number;
        uint64_t primary_key()const {return round;}
    };
    typedef eosio::multi_index<"rounds"_n, round_item> rounds_table;
    
    struct [[eosio::table]] round_result {
        uint64_t bill_id;
        name player;
        std::string data;
        std::string lucky_number;
        asset    reward;
        uint64_t primary_key()const {return bill_id;}
    };
    typedef eosio::multi_index<"roundresults"_n, round_result> results_table;
    
    
    global_singleton _global;
    global_item _global_state;
    rounds_table _rounds;
    results_table _results;
};
