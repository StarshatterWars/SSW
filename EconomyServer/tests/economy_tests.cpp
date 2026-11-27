#include "ssw/economy.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <chrono>
namespace {int checks=0;void check(bool value,const std::string& reason){if(!value)throw std::runtime_error(reason);++checks;}
template<class F>void rejects(F&& operation,const std::string& reason){bool rejected=false;try{operation();}catch(const std::exception&){rejected=true;}check(rejected,reason);}}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::runtime_error("GameData path required");const auto catalog=ssw::Catalog::load(argv[1]);
    check(catalog.goods.size()>=270,"Full SSW cargo catalog");check(catalog.recipes.size()>=223,"Full recipe catalog");check(catalog.facilities.size()>=52,"Full facilities catalog");check(catalog.mines.size()>=100,"All authored mines");check(!catalog.route("Janus","Solus").empty(),"Authored jump graph route");check(catalog.route("Missing","Janus").empty(),"Unknown route rejected");
    ssw::World world(catalog);const auto initial=world.state();rejects([&]{world.advance(-1);},"Negative time rejected");rejects([&]{world.advance(std::numeric_limits<double>::quiet_NaN());},"Non-finite time rejected");
    const auto root=std::filesystem::temp_directory_path()/("SSW economy tests "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(root);const auto save=root/"economy.json";
    world.advance(10800);world.checkpoint(save);ssw::World restored(catalog);check(restored.load_checkpoint(save),"Checkpoint restored");check(restored.state()==world.state(),"Every inventory/job/journey/clock round trips");restored.advance(10800);restored.validate();const auto status=restored.status();std::cout<<status.dump(2)<<'\n';check(status.at("completedJobs").get<std::size_t>()>0,"Unloaded native economy produces goods");check(status.at("deliveredFreight").get<std::size_t>()>0,"Freight unloads before destination credit");
    ssw::World continuous(catalog);continuous.advance(21600);check(continuous.state()==restored.state(),"Restart retains deterministic six-hour progression");
    ssw::World player(catalog);const auto create=ssw::json{{"id","create-1"},{"action","create_account"},{"actor","Pilot/Fixture"},{"system","Janus"},{"capacityTonnes",48}};player.execute(create);const auto once=player.state();player.execute(create);check(player.state()==once,"Account command retry cannot mint starter credits twice");check(player.state()["wallets"]["Pilot/Fixture"]==5000,"SSW starting balance retained");
    const auto purchase=ssw::json{{"id","purchase-1"},{"action","buy"},{"actor","Pilot/Fixture"},{"market","Janus/NPC Market/Stock"},{"item","industrial concentrate"},{"quantity",1}};
    const auto quote=player.execute(purchase);check(quote.at("totalCredits").get<std::int64_t>()>quote.at("baseCredits").get<std::int64_t>(),"Authored tax and fee charged");check(quote.at("baseCredits").get<std::int64_t>()+quote.at("feesCredits").get<std::int64_t>()==quote.at("totalCredits").get<std::int64_t>(),"SSW GST is included in the quoted final price");const auto total=quote.at("totalCredits").get<std::int64_t>();check(quote.at("feesCredits").get<std::int64_t>()==(total/10200)*200+((total%10200)*200)/10200,"SSW whole-credit included GST rounding");const auto bought=player.state();player.execute(purchase);check(player.state()==bought,"Purchase retries cannot charge/deposit twice");
    auto invalid=purchase;invalid["id"]="negative";invalid["quantity"]=-1;rejects([&]{player.execute(invalid);},"Negative purchase rejected");check(player.state()==bought,"Rejected command rolls back every field");invalid["id"]="stale";invalid["quantity"]=1;invalid["expectedRevision"]=0;rejects([&]{player.execute(invalid);},"Stale revision rejected");check(player.state()==bought,"Stale rejection preserves ledger");
    restored.checkpoint(save);{std::ofstream broken(save);broken<<"broken";}ssw::World recovery(catalog);check(recovery.load_checkpoint(save),"Corrupt current checkpoint recovers previous complete generation");recovery.validate();{std::ofstream broken(save.string()+".bak");broken<<"broken";}rejects([&]{recovery.load_checkpoint(save);},"Both corrupt checkpoints fail closed instead of resetting stock");
    // Delete only this test-owned directory, whose complete path was constructed above.
    std::filesystem::remove_all(root);std::cout<<checks<<" native economy checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}}
