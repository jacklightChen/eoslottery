#include "lottery.hpp"

lottery4test::global_item lottery4test::get_default_parameters() {
    global_item global;
    global.key_price = asset(1000, EOS_SYMBOL);  //0.1 EOS
    global.cur_round = 0;
    global.total_claimed_token_bonus = asset(0, EOS_SYMBOL);
    global.total_users = 0;
    return global;
}

void lottery4test::active(name actor) {
    check(_global_state.active == false, "lottery can't be actived twice!");
    require_auth(actor);

    SEND_INLINE_ACTION(*this, setactived, {_self, "active"_n}, {true});

    newround(actor);
}

void lottery4test::setactived(bool actived) {
    require_auth(_self);
    _global_state.active = actived;
}

void lottery4test::newround(name actor) {
    uint64_t next = _global_state.cur_round + 1;
    check(next > _global_state.cur_round, "new round number is smaller than or equal with the old!");

    asset next_bucket = asset(0, EOS_SYMBOL);
    auto rnd_pos = _rounds.find(_global_state.cur_round);
    auto fee_pos = _roundfee.find(_global_state.cur_round);
    if (rnd_pos != _rounds.end() && fee_pos != _roundfee.end() && rnd_pos->lucky_player.value <= 0) {
        next_bucket = fee_pos->to_lucky;
    }

    _rounds.emplace(actor, [&](round_item &info) {
        info.round = next;
        info.bills_num = 0;
        info.last_key = 0;
        info.reward_bucket = next_bucket;
        info.start_time = current_time_point().sec_since_epoch();
        info.draw_time = 0;
    });

    _global_state.cur_round = next;
}

void lottery4test::endround() {
    round_item rnd = _rounds.get(_global_state.cur_round);

    bills_table bills(_self, _global_state.cur_round);
    auto to_lucky = rnd.reward_bucket;
    if (rnd.lucky_player.value > 0) {
        char buffer[256];
        sprintf(buffer, "[bysj_lottery][round: %lld] winner reward", _global_state.cur_round);
        std::string notify = buffer;
        action(
                permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), rnd.lucky_player, to_lucky, notify)
        ).send();
    }

    _roundfee.emplace(_self, [&](auto &info) {
        info.round = _global_state.cur_round;
        info.to_lucky = to_lucky;
    });
}

void lottery4test::transfer(name from, name to, asset quantity, std::string memo) {
    if (from == _self || to != _self) {
        return;
    }
    uint64_t num = static_cast<uint64_t>(quantity.amount / _global_state.key_price.amount);
    check(num * _global_state.key_price.amount == quantity.amount,
          "transfer number must be integer multiples of key price");
    bills_table bills(get_self(), _global_state.cur_round);
    bills.emplace(_self, [&](bill_item &info) {
        info.id = bills.available_primary_key();
        info.player = from;
        info.low = quantity.amount;
        info.high = 0;
        info.time = current_time_point().sec_since_epoch();
    });

    check(_global_state.active == true, "lottery not activated yet!");
    check(quantity.symbol == EOS_SYMBOL, "accept EOS only");
    check(quantity.is_valid(), "transfer invalid quantity");
    check(quantity.amount >= _global_state.key_price.amount, "amount can't be smaller than the key price");

    bill_item bill = buykeys(from, quantity, memo);

    auto pos = _rounds.find(_global_state.cur_round);
    check(pos != _rounds.end(), "round number not found");

    _rounds.modify(pos, get_self(), [&](round_item &info) {
        info.reward_bucket += quantity;
        info.bills_num += 1;
    });
}

lottery4test::bill_item lottery4test::buykeys(name buyer, asset quantity, std::string memo) {
    bill_item bill;
    bill.player = buyer;
    auto rnd = _rounds.find(_global_state.cur_round);
    check(rnd != _rounds.end(), "round number can't be found in rounds table error!");

    uint64_t num = static_cast<uint64_t>(quantity.amount / _global_state.key_price.amount);
    check(num * _global_state.key_price.amount == quantity.amount,
          "transfer number must be integer multiples of key price");

    bill.low = rnd->last_key + 1 + num;
    check(bill.low > rnd->last_key, "low key is out of range while buy keys");

    bill.high = bill.low + num - 1;
    check(bill.high > rnd->last_key, "high key is out of range while buy keys");

    bill.time = current_time_point().sec_since_epoch();

    bills_table bills(_self, _global_state.cur_round);
    bill.id = bills.available_primary_key();
    bills.emplace(_self, [&](bill_item &info) {
        info = bill;
    });

    _rounds.modify(rnd, buyer, [&](round_item &info) {
        info.last_key = bill.high;
    });

    _global_state.total_users += 1;

    return bill;
}


uint64_t lottery4test::randomkey(uint64_t max) {
    checksum256 result;
    static uint64_t seed = static_cast<uint64_t>((int) &result);
    auto rnd = _rounds.find(_global_state.cur_round);
    auto mixedBlock = tapos_block_prefix() + tapos_block_num() + rnd->reward_bucket.amount + seed +
                      current_time_point().sec_since_epoch();;

    seed += (mixedBlock >> 33);

    const char *mixedChar = reinterpret_cast<const char *>(&mixedBlock);
    result = sha256((char *) mixedChar, sizeof(mixedChar));
    const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&result);

    check(max > 0, "random max must > 0");
    return (p64[1] % max) + 1;
}

void lottery4test::delaydraw(name drawer) {
    require_auth(drawer);

    check(_global_state.active == true, "lottery has not been actived!");

    auto pos = _rounds.find(_global_state.cur_round);
    check(pos != _rounds.end(), "round is missing!");
    check(pos->last_key > 0, "none buy keys");

    auto ct = current_time_point().sec_since_epoch();;
    check(ct > (pos->start_time + useconds_draw_interval), "draw time has not cool down!");

    // players_table players(_self, _global_state.cur_round);
    // players.get(drawer, "join game first!");

    transaction dr;
    dr.actions.emplace_back(permission_level{_self, "active"_n}, _self, "drawing"_n, drawer);
    dr.delay_sec = 1;
    dr.send(get_self().value, _self);
}

void lottery4test::drawing(name drawer) {
    require_auth(_self);

    transaction dr;
    dr.actions.emplace_back(permission_level{_self, "active"_n}, _self, "draw"_n, drawer);
    dr.delay_sec = 1;
    dr.send(get_self().value, get_self());
}

void lottery4test::draw(name drawer) {
    require_auth(_self);

    auto pos = _rounds.find(_global_state.cur_round);

    auto ct = current_time_point().sec_since_epoch();

    uint64_t lucky_key = randomkey(pos->last_key);

    bills_table bills(_self, _global_state.cur_round);
    auto it = bills.begin();
    _rounds.modify(pos, get_self(), [&](round_item &info) {
        info.draw_account = drawer;
        info.draw_time = ct;
        info.lucky_key = lucky_key;

        if (it == bills.end()) {
            info.lucky_player = "active"_n;
        } else {
            info.lucky_player = it->player;
        }

    });

    endround();
    newround(_self);
}

#ifdef EOSIO_DISPATCH
#undef EOSIO_DISPATCH
#endif
#define EOSIO_DISPATCH(TYPE, MEMBERS) extern "C" {                                                                                  \
void apply(uint64_t receiver, uint64_t code, uint64_t action) {                               \
  if ( code == receiver ) {                                                                   \
    switch( action ) {                                                                        \
      EOSIO_DISPATCH_HELPER( TYPE, MEMBERS )                                                  \
    }                                                                                         \
  }                                                                                           \
  if (code == "eosio.token"_n.value && action == "transfer"_n.value) {                        \
    eosio::execute_action(eosio::name(receiver), eosio::name(code), &lottery4test::transfer); \
    return;                                                                                   \
  }                                                                                           \
  if (action == "transfer"_n.value) {                                                         \
    check(false, "only support EOS token");                                                   \
  }                                                                                           \
}                                                                                             \
}