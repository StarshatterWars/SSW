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
    const auto quote=player.execute(purchase);check(quote.at("totalCredits").get<std::int64_t>()>quote.at("baseCredits").get<std::int64_t>(),"Authored tax and fee charged");check(quote.at("baseCredits").get<std::int64_t>()+quote.at("feesCredits").get<std::int64_t>()==quote.at("totalCredits").get<std::int64_t>(),"SSW GST is included in the quoted final price");const auto total=quote.at("totalCredits").get<std::int64_t>();check(quote.at("feesCredits").get<std::int64_t>()==(total/10200)*200+((total%10200)*200)/10200,"SSW whole-credit included GST rounding");const auto bought=player.state();player.execute(purchase);check(player.state()==bought,"Purchase retries cannot charge/deposit twice");auto aliased=purchase;aliased["quantity"]=2;rejects([&]{player.execute(aliased);},"A receipt cannot be reused for a different command");check(player.state()==bought,"Receipt collision leaves all state unchanged");
    ssw::World shared(catalog);const std::string tradeActor="Pilot/ExternalFixture",market="Janus/NPC Market/Stock",cargo=catalog.item("industrial concentrate");
    auto prep=ssw::json{{"id","prepare-external-buy"},{"action","prepare_external_trade"},{"tradeId","external-buy"},{"actor",tradeActor},{"market",market},{"side","Buy"},{"item",cargo},{"quantity",1},{"expectedTotal",quote.at("totalCredits")}};
    const double stock=shared.state()["inventories"][market]["stock"][cargo];const auto prepared=shared.execute(prep);
    check(prepared.at("state")=="Prepared","NPC trade reserves before player settlement");check(shared.state()["inventories"][market]["stock"][cargo]==stock,"Preparing does not deliver or remove stock");
    check(!shared.state()["wallets"].contains(tradeActor),"Player wallet remains outside the NPC economy");const auto preparedState=shared.state();shared.execute(prep);check(shared.state()==preparedState,"Prepared trade retries reserve once");
    const auto externalSave=root/"external.json";shared.checkpoint(externalSave);ssw::World resumed(catalog);check(resumed.load_checkpoint(externalSave),"Prepared external trade survives restart");
    const auto readStatus=ssw::json{{"id","stable-status-read"},{"action","external_trade_status"},{"tradeId","external-buy"},{"actor",tradeActor}};
    check(resumed.execute(readStatus).at("state")=="Prepared","Trade status reads the current prepared state");
    auto settle=ssw::json{{"id","settle-external-buy"},{"action","settle_external_trade"},{"tradeId","external-buy"},{"actor",tradeActor},{"playerEscrowId","external-buy"}};
    check(resumed.execute(settle).at("state")=="Committed","Durable player escrow permits NPC settlement");check(resumed.execute(readStatus).at("state")=="Committed","Repeated status request never returns stale cached preparation");const auto settled=resumed.state();resumed.execute(settle);check(resumed.state()==settled,"Settlement retry cannot exchange twice");
    check(resumed.state()["inventories"][market]["stock"][cargo].get<double>()==stock-1,"NPC stock removed exactly once at commitment");
    auto cancel=ssw::json{{"id","cancel-after-settlement"},{"action","cancel_external_trade"},{"tradeId","external-buy"},{"actor",tradeActor}};
    check(resumed.execute(cancel).at("state")=="Committed","Committed trade cannot be cancelled into a refund");
    cancel["id"]="other-pilot";cancel["actor"]="Pilot/Other";rejects([&]{resumed.execute(cancel);},"Another player cannot operate a receipt");
    ssw::World priceProbe(catalog);priceProbe.restore(resumed.state());auto probe=create;probe["id"]="probe-create";probe["actor"]="Pilot/Probe";priceProbe.execute(probe);auto probeBuy=purchase;probeBuy["id"]="probe-price";probeBuy["actor"]="Pilot/Probe";const auto sellTotal=priceProbe.execute(probeBuy).at("totalCredits");
    prep["id"]="prepare-external-sell";prep["tradeId"]="external-sell";prep["side"]="Sell";prep["expectedTotal"]=sellTotal;
    const std::string npc=resumed.state()["inventories"][market]["owner"];const auto npcBefore=resumed.state()["wallets"][npc].get<std::int64_t>();
    const auto sell=resumed.execute(prep);check(resumed.state()["wallets"][npc].get<std::int64_t>()==npcBefore-sell.at("totalCredits").get<std::int64_t>(),"NPC buy budget is escrowed before player delivery");
    cancel["id"]="cancel-external-sell";cancel["tradeId"]="external-sell";cancel["actor"]=tradeActor;check(resumed.execute(cancel).at("state")=="Cancelled","Prepared sale can be cancelled");check(resumed.state()["wallets"][npc].get<std::int64_t>()==npcBefore,"Cancellation refunds NPC escrow once");
    const auto cancelled=resumed.state();resumed.execute(cancel);check(resumed.state()==cancelled,"Cancellation retries do not mint NPC funds");resumed.validate();
    ssw::World expiring(catalog);prep["id"]="expiring-prepare";prep["tradeId"]="expiring";prep["side"]="Buy";prep["expectedTotal"]=quote.at("totalCredits");expiring.execute(prep);expiring.advance(301);
    check(expiring.state()["externalTrades"]["expiring"]["state"]=="Cancelled","Abandoned preparation expires without settlement");check(expiring.state()["inventories"][market]["reservedPickup"].value(cargo,0.0)==0,"Expired preparation releases stock for the economy");
    auto invalid=purchase;invalid["id"]="negative";invalid["quantity"]=-1;rejects([&]{player.execute(invalid);},"Negative purchase rejected");check(player.state()==bought,"Rejected command rolls back every field");invalid["id"]="stale";invalid["quantity"]=1;invalid["expectedRevision"]=0;rejects([&]{player.execute(invalid);},"Stale revision rejected");check(player.state()==bought,"Stale rejection preserves ledger");
    restored.checkpoint(save);{std::ofstream broken(save);broken<<"broken";}ssw::World recovery(catalog);check(recovery.load_checkpoint(save),"Corrupt current checkpoint recovers previous complete generation");recovery.validate();{std::ofstream broken(save.string()+".bak");broken<<"broken";}rejects([&]{recovery.load_checkpoint(save);},"Both corrupt checkpoints fail closed instead of resetting stock");
    ssw::World target(catalog);auto standby=target.state();standby["worldId"]="handoff-test-world";target.restore(standby);
    auto handoff=ssw::json{{"action","bootstrap_npc"},{"format","SSW-NPC-1"},{"handoffId","source-cutover-1"},{"sourceId","local-world"},{"snapshot",world.state()}};
    auto activation=target.execute(handoff);check(activation.at("worldId")=="handoff-test-world","Cutover uses the service world identity");
    check(target.status().at("npcAuthorityActive")==true,"Valid cutover activates NPC authority");
    const auto activated=target.state();check(target.execute(handoff)==activation&&target.state()==activated,"Lost activation response retries without restoring old stock");
    auto conflict=handoff;conflict["handoffId"]="different-source";rejects([&]{target.execute(conflict);},"Another source cannot overwrite an active world");check(target.state()==activated,"Rejected replacement preserves every field");
    target.checkpoint(root/"handoff.json");ssw::World reconnect(catalog);reconnect.load_checkpoint(root/"handoff.json");check(reconnect.execute(handoff)==activation,"Reconnect retains activation identity and receipt");
    target.checkpoint(root/"handoff.json");{std::ofstream corrupt(root/"handoff.json");corrupt<<"corrupt";}
    rejects([&]{reconnect.load_checkpoint(root/"handoff.json");},"Live checkpoint corruption cannot silently resurrect older stock and settled receipts");
    std::filesystem::remove(root/"handoff.json");std::filesystem::remove(root/"handoff.json.bak");
    rejects([&]{reconnect.load_checkpoint(root/"handoff.json");},"A retained live fence prevents reseeding when both checkpoints are missing");
    ssw::World invalidTarget(catalog);auto unsafe=handoff;unsafe["snapshot"]["wallets"]["Pilot/ShouldStayLocal"]=5000;const auto invalidBefore=invalidTarget.state();rejects([&]{invalidTarget.execute(unsafe);},"Player assets cannot enter an NPC handoff");check(invalidTarget.state()==invalidBefore,"Invalid handoff rolls back before activation");
    // Delete only this test-owned directory, whose complete path was constructed above.
    std::filesystem::remove_all(root);std::cout<<checks<<" native economy checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}}
