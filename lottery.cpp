#include "lottery.hpp"

lottery::global_item lottery::get_default_parameters() {
    global_item global;
    global.key_price = asset(1000, EOS_SYMBOL);  //0.1 EOS
    global.cur_round = 0;
    global.total_users = 0;
    return global;
}

void lottery::active(name actor) {
    check(_global_state.active == false, "lottery can't be actived twice!");
    require_auth(actor);

    SEND_INLINE_ACTION(*this, setactived, {get_self(), "active"_n}, {true});

    newround(actor);
}

void lottery::setactived(bool actived) {
    require_auth(get_self());
    _global_state.active = actived;
}

void lottery::newround(name actor) {
    uint64_t next = _global_state.cur_round + 1;
    check(next > _global_state.cur_round, "new round number is smaller than or equal with the old!");

    asset next_bucket = asset(0, EOS_SYMBOL);
    auto rnd = _rounds.find(_global_state.cur_round);
    if (rnd != _rounds.end()) {
        next_bucket = rnd->reward_bucket;
    }

    _rounds.emplace(actor, [&](round_item &info) {
        info.round = next;
        info.bills_num = 0;
        info.reward_bucket = next_bucket;
        info.start_time = current_time_point().sec_since_epoch();
        info.draw_time = 0;
    });

    _global_state.cur_round = next;
}

void lottery::endround() {
    asset next_bucket = asset(0, EOS_SYMBOL);
    asset reward_sum = asset(0, EOS_SYMBOL);
    auto rnd = _rounds.find(_global_state.cur_round);
    check(rnd != _rounds.end(), "round is missing!");

    next_bucket = rnd->reward_bucket;

    std::vector <std::string> win_number;
    win_number = spilt(rnd->lucky_number, ",");
    int first_prize_num = 0;
    int second_prize_num = 0;
    int third_prize_num = 0;
    int fourth_prize_num = 0;


    bills_table bills(get_self(), _global_state.cur_round);
    auto it = bills.begin();
    for (; it != bills.end(); it++) {
        std::vector <std::string> data;
        data = spilt(it->data, ",");
        int cnt = 0;
        for (int i = 0; i < 4; i++) {
            if (data[i] == win_number[i]) {
                cnt++;
            }
        }
        switch (cnt) {
            case 4:
                first_prize_num++;
                break;
            case 3:
                second_prize_num++;
                break;
            case 2:
                third_prize_num++;
                break;
            case 1:
                fourth_prize_num++;
                break;
            default:
                break;
        }
    }

    it = bills.begin();
    for (; it != bills.end(); it++) {
        std::vector <std::string> data;
        data = spilt(it->data, ",");
        int cnt = 0;
        for (int i = 0; i < 4; i++) {
            if (data[i] == win_number[i]) {
                cnt++;
            }
        }

        double share = 0;
        int division = 1;

        switch (cnt) {
            case 4:
                share = FIRST_PRIZE_SHARE;
                division = first_prize_num;
                break;
            case 3:
                share = SECOND_PRIZE_SHARE;
                division = second_prize_num;
                break;
            case 2:
                share = THIRD_PRIZE_SHARE;
                division = third_prize_num;
                break;
            case 1:
                share = FOURTH_PRIZE_SHARE;
                division = fourth_prize_num;
                break;
            default:
                break;
        }

        if (share > 0) {
            action(
                    permission_level{get_self(), "active"_n},
                    "eosio.token"_n,
                    "transfer"_n,
                    std::make_tuple(get_self(), it->player, asset(next_bucket.amount * share / division, EOS_SYMBOL),
                                    std::string("reward for bill_id: ") + std::to_string(it->bill_id))
            ).send();
            reward_sum = reward_sum + asset(next_bucket.amount * share / division, EOS_SYMBOL);
        }

        _results.emplace(get_self(), [&](round_result &info) {
            info.bill_id = it->bill_id;
            info.player = it->player;
            info.data = it->data;
            info.lucky_number = rnd->lucky_number;
            info.reward = asset(next_bucket.amount * share / division, EOS_SYMBOL);
        });
    }

    // resolve result
    _rounds.modify(rnd, get_self(), [&](round_item &info) {
        info.reward_bucket -= reward_sum;
    });
}

void lottery::transfer(name from, name to, asset quantity, std::string memo) {
    if (from == get_self() || to != get_self()) {
        return;
    }
    uint64_t num = static_cast<uint64_t>(quantity.amount / _global_state.key_price.amount);
    check(num * _global_state.key_price.amount == quantity.amount,
          "transfer number must be integer multiples of key price");
    std::vector <std::string> data;
    data = spilt(memo, ",");
    check(data.size() == 6, "the count of ticket number should be exactly six");
    std::vector<int> ticket;
    for (auto s:data) {
        check(stoi(s), "number format failure");
        ticket.push_back(stoi(s));
    }

    check(_global_state.active == true, "lottery not activated yet!");
    check(quantity.symbol == EOS_SYMBOL, "accept EOS only");
    check(quantity.is_valid(), "transfer invalid quantity");
    check(quantity.amount >= _global_state.key_price.amount, "amount can't be smaller than the key price");

    buykeys(from, quantity, memo);

    auto rnd = _rounds.find(_global_state.cur_round);
    check(rnd != _rounds.end(), "round number not found");

    _rounds.modify(rnd, get_self(), [&](round_item &info) {
        info.reward_bucket += quantity;
        info.bills_num += 1;
    });
}

void lottery::buykeys(name buyer, asset quantity, std::string memo) {
    auto rnd = _rounds.find(_global_state.cur_round);
    check(rnd != _rounds.end(), "round number can't be found in rounds table error!");

    auto ct = current_time_point().sec_since_epoch();
    check(ct < (rnd->start_time + useconds_draw_interval), "this round is end!");

    bills_table bills(get_self(), _global_state.cur_round);
    bills.emplace(get_self(), [&](bill_item &info) {
        info.bill_id = bills.available_primary_key();
        info.player = buyer;
        info.data = memo;
        info.time = ct;
    });
    _global_state.total_users += 1;
}

//get sha256 hash
checksum256 lottery::randomkey() {
    checksum256 result;
    auto draw_time = current_time_point().sec_since_epoch();
    auto rnd = _rounds.find(_global_state.cur_round);
    check(rnd != _rounds.end(), "round number can't be found in rounds table error!");
    _rounds.modify(rnd, get_self(), [&](round_item &info) {
        info.draw_time = draw_time;
    });
    auto mixedBlock = tapos_block_prefix() + tapos_block_num() + current_time_point().sec_since_epoch() + draw_time;
    const char *mixedChar = reinterpret_cast<const char *>(&mixedBlock);
    result = sha256((char *) mixedChar, sizeof(mixedChar));
    return result;
}

//form lottery number
std::string lottery::keytostring(checksum256 randomkey) {
    auto checksumBytes = randomkey.extract_as_byte_array().data();
    uint64_t num1, num2, num3, num4;
    memcpy(&num1, &checksumBytes[0], sizeof(num1));
    memcpy(&num2, &checksumBytes[7], sizeof(num2));
    memcpy(&num3, &checksumBytes[14], sizeof(num3));
    memcpy(&num4, &checksumBytes[21], sizeof(num4));
    num1 = (num1 % 33) + 1;
    num2 = (num2 % 33) + 1;
    num3 = (num3 % 33) + 1;
    num4 = (num4 % 33) + 1;
    std::string lottery_number =
            std::to_string(num1) + std::string(",") + std::to_string(num2) + std::string(",") + std::to_string(num3) +
            std::string(",") + std::to_string(num4);
    return lottery_number;
}

void lottery::delaydraw() {
    require_auth(get_self());

    check(_global_state.active == true, "lottery has not been actived!");

    auto rnd = _rounds.find(_global_state.cur_round);
    check(rnd != _rounds.end(), "round is missing!");

    auto ct = current_time_point().sec_since_epoch();
    check(ct > (rnd->start_time + useconds_draw_interval), "draw time has not cool down!");

    transaction dr;
    dr.actions.emplace_back(permission_level{get_self(), "active"_n}, get_self(), "drawing"_n,
                            std::make_tuple(get_self()));
    dr.delay_sec = 1;
    dr.send(get_self().value, get_self());
}

void lottery::drawing(name drawer) {
    require_auth(get_self());
    transaction dr;
    dr.actions.emplace_back(permission_level{get_self(), "active"_n}, get_self(), "draw"_n,
                            std::make_tuple(get_self()));
    dr.delay_sec = 1;
    dr.send(get_self().value, get_self());
}

void lottery::draw(name drawer) {
    require_auth(get_self());
    checksum256 lucky_key = randomkey();

    auto rnd = _rounds.find(_global_state.cur_round);
    check(rnd != _rounds.end(), "round is missing!");
    _rounds.modify(rnd, get_self(), [&](round_item &info) {
        info.lucky_key = lucky_key;
        info.lucky_number = keytostring(lucky_key);
    });

    endround();
    newround(get_self());
}

std::vector <std::string> lottery::spilt(const std::string &str, const std::string &delim) {
    std::vector <std::string> spiltCollection;
    if (str.size() == 0)
        return spiltCollection;
    int start = 0;
    int idx = str.find(delim, start);
    while (idx != std::string::npos) {
        spiltCollection.push_back(str.substr(start, idx - start));
        start = idx + delim.size();
        idx = str.find(delim, start);
    }
    spiltCollection.push_back(str.substr(start));
    return spiltCollection;
}

#ifdef EOSIO_DISPATCH
#undef EOSIO_DISPATCH
#endif
#define EOSIO_DISPATCH(TYPE, MEMBERS)                                    \
extern "C" {                                                               \
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {        \
        if ( code == receiver ) {                                          \
            switch( action ) {                                             \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS )                     \
            }                                                              \
        }                                                                  \
        if (code == "eosio.token"_n.value && action == "transfer"_n.value) { \
            execute_action(eosio::name(receiver), eosio::name(code), &lottery::transfer);   \
            return;                                                        \
        }                                                                  \
        if (action == "transfer"_n.value) {                                 \
            check(false, "only support EOS token");                 \
        }                                                                  \
    }                                                                      \
}

EOSIO_DISPATCH(lottery, (active)(setactived)(delaydraw)(drawing)(draw))

