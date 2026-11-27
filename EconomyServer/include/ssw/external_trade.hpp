#pragma once
#include "economy.hpp"
namespace ssw {
// Local trusted player adapter: player assets stay outside the NPC world.
// The caller durably escrows its side before requesting settlement.
std::int64_t external_unit_price(const json&,const Catalog&,const std::string&,const std::string&);
json quote_external_trade(const json& world,const Catalog&,const json& command);
json external_trade(json& world,const Catalog&,const json& command);
void expire_external_trades(json& world,const Catalog&);
}
