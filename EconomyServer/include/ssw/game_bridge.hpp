#pragma once
#include "economy.hpp"
namespace ssw {
json import_game(Catalog&,const json& seed,const json& payload);
json project_game(const Catalog&,const json& state);
void register_game_sites(Catalog&,json&,const json&);
void restore_game_facilities(Catalog&,const json& state);
}
