#include "ssw/economy.hpp"
#include "ssw/game_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <fstream>
namespace {int checks=0;void check(bool v,const char* why){if(!v)throw std::runtime_error(why);++checks;}}
int main(int argc,char** argv){try{
    if(argc!=3)throw std::runtime_error("Catalog and actual UE fixture required");auto catalog=ssw::Catalog::load(argv[1]);auto fixture=ssw::read_json(argv[2]);
    ssw::World world(catalog);auto seed=world.state();seed["worldId"]="ue-integration-world";world.restore(seed);
    const auto request=ssw::json{{"action","bootstrap_npc"},{"format","SSW-UE-NPC-1"},{"sourceId","isolated-UE-fixture"},{"handoffId","actual-UE-wire"},{"snapshot",fixture}};
    auto accepted=world.execute(request);check(accepted.at("ok"),"Actual UE snapshot accepted");auto projection=world.execute({{"action","npc_projection"},{"worldId","ue-integration-world"}});
    check(!projection["economy"]["wallets"].contains("Pilot"),"Player wallet never enters service projection");
    check(!projection["economy"]["inventories"].contains("Pilot/Cargo"),"Player cargo never enters service projection");
    bool exact=false;for(const auto& [owner,balance]:projection["economy"]["wallets"].items()){(void)owner;if(balance=="9007199254740993")exact=true;}check(exact,"Real UE serialization retains int64 credits above double precision");
    check(world.state()["jobs"].size()==1&&world.state()["jobs"][0]["status"]=="Running","Running NPC job migrated without consuming inputs again");
    check(world.state()["journeys"].size()==1&&world.state()["journeys"][0]["status"]=="Transit","Loaded freight remains in transit");
    const auto trip=world.state()["journeys"][0];const auto dest=trip.at("destination").get<std::string>(),hold=trip.at("hold").get<std::string>(),item=trip.at("item").get<std::string>();
    const double initial=world.state()["inventories"][dest]["stock"].value(item,0.0);
    check(world.state()["inventories"][hold]["stock"].value(item,0.0)==trip.at("quantity").get<double>(),"Migrated freight has actual held cargo");
    world.advance(16);check(world.state()["deliveredFreight"].get<int>()>=1,"Migrated cargo credited only at server unload");
    check(world.state()["inventories"][hold]["stock"].value(item,0.0)==0,"Unloaded cargo removed from its source hold once");
    check(world.state()["inventories"][dest]["stock"].value(item,0.0)==initial+trip.at("quantity").get<double>(),"Destination receives exact migrated shipment");
    world.advance(15);check(world.state()["completedBatches"].get<int>()>=1,"Migrated production completes on its remaining deadline");
    const auto root=std::filesystem::path(argv[2]).parent_path();const auto checkpoint=root/"EconomyWireIntegrationCheckpoint.json";
    world.checkpoint(checkpoint);ssw::World recovered(catalog);check(recovered.load_checkpoint(checkpoint),"Live UE-derived checkpoint restored");check(recovered.state()==world.state(),"All NPC records retain identity across restart");
    recovered.advance(3600);check(recovered.state()["completedBatches"].get<int>()>1,"Unloaded server keeps processing the Janus loop");recovered.validate();
    const auto final=recovered.execute({{"action","npc_projection"},{"worldId","ue-integration-world"}});
    check(final.at("traffic").at("journeys").size()>0,"Server publishes transport view data");
    auto output=root/"EconomyWireProjection.json";{std::ofstream file(output);file<<final.dump();}
    for(auto suffix:{"",".bak",".live"})std::filesystem::remove(std::filesystem::path(checkpoint.string()+suffix));
    std::cout<<checks<<" actual UE/native bridge checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
