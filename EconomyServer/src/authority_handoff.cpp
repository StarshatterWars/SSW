#include "ssw/economy.hpp"
#include <stdexcept>
namespace ssw {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}
json World::bootstrap(const json& command){
    const std::string handoff=command.at("handoffId"),source=command.at("sourceId");
    require(!handoff.empty()&&handoff.size()<=128&&!source.empty()&&source.size()<=128,"Invalid handoff identity");
    const auto& incoming=command.at("snapshot");
    // Compare the complete canonical payload, not an implementation-dependent hash.
    if(state_.value("npcAuthorityActive",false)){
        require(state_.value("handoffId",std::string())==handoff&&state_.value("sourceId",std::string())==source&&state_.contains("handoffSnapshot")&&state_["handoffSnapshot"]==incoming,"This service already owns another handoff; refusing to overwrite live stock");
        return {{"ok",true},{"worldId",state_.at("worldId")},{"handoffId",handoff},{"revision",state_.at("revision")}};
    }
    require(command.value("format",std::string())=="SSW-NPC-1","Unsupported NPC handoff format");
    require(incoming.is_object()&&incoming.value("schema",0)==1,"Invalid NPC handoff snapshot");
    const auto before=state_;
    try{
        // Restoring validates quantities, capacity and reservations before activation.
        restore(incoming);
        require(state_.contains("mines")&&state_.contains("jobs")&&state_.contains("journeys")&&state_.contains("history"),"Incomplete NPC handoff");
        for(const auto& [owner,balance]:state_.at("wallets").items()){
            (void)balance;require(owner=="Treasury"||owner.starts_with("NPC/")||owner.starts_with("Residents/"),"Only explicit NPC budgets may enter an NPC handoff");
        }
        for(const auto& [key,inv]:state_.at("inventories").items()){
            (void)key;const auto owner=inv.at("owner").get<std::string>();
            require(owner.starts_with("NPC/"),"Only explicit NPC inventories may enter an NPC handoff");
        }
        for(const auto& mine:catalog_.mines){
            const auto& record=state_.at("mines").at(mine.id);
            require(record.at("remaining").is_number()&&record.at("remaining").get<double>()>=0,"Invalid remaining ore");
            require(state_.at("inventories").contains(record.at("inventory").get<std::string>()),"Missing mine inventory");
        }
        for(const auto& [id,facility]:catalog_.facilities){
            (void)id;const auto base=facility.system+"/"+facility.name;
            require(state_.at("inventories").contains(base+"/Input")&&state_.at("inventories").contains(base+"/Output"),"Missing facility warehouses");
        }
        for(const auto& job:state_.at("jobs")){
            require(catalog_.facilities.contains(job.at("facility").get<std::string>())&&catalog_.recipes.contains(job.at("recipe").get<std::string>()),"Unmapped active industry job");
        }
        // A standby service has not advanced; preserve its newly generated world identity.
        state_["worldId"]=before.at("worldId");state_["npcAuthorityActive"]=true;
        state_["handoffId"]=handoff;state_["sourceId"]=source;state_["handoffSnapshot"]=incoming;
        state_["revision"]=before.at("revision").get<std::uint64_t>()+1;
        validate();
        return {{"ok",true},{"worldId",state_.at("worldId")},{"handoffId",handoff},{"revision",state_.at("revision")}};
    }catch(...){state_=before;throw;}
}
}
