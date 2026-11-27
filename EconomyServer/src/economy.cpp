#include "ssw/economy.hpp"
#include "ssw/external_trade.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace ssw {
namespace {
constexpr double epsilon=1.e-7;
std::string normalized(std::string text){
    std::string result;for(unsigned char ch:text)if(std::isalnum(ch))result+=static_cast<char>(std::tolower(ch));return result;
}
void require(bool value,const std::string& reason){if(!value)throw std::runtime_error(reason);}
double positive(double value,const std::string& name){require(std::isfinite(value)&&value>0,name+" must be finite and positive");return value;}
std::int64_t credits(double value){require(std::isfinite(value)&&value>=0&&value<9.e15,"Credit amount out of range");return static_cast<std::int64_t>(std::ceil(value));}
std::string warehouse(const Facility& facility,bool output){return facility.system+"/"+facility.name+(output?"/Output":"/Input");}
bool contains(const std::vector<std::string>& values,const std::string& value){return std::find(values.begin(),values.end(),value)!=values.end();}
double factor(double stock,double demand){return std::clamp(.8+std::max(1.0,demand)/std::max(1.0,stock),.85,3.0);}
}
json read_json(const std::filesystem::path& path){std::ifstream file(path);if(!file){const auto utf8=path.u8string();throw std::runtime_error("Cannot read catalog: "+std::string(utf8.begin(),utf8.end()));}json data;file>>data;return data;}
std::string digest(const std::string& text){std::uint64_t hash=14695981039346656037ull;for(unsigned char ch:text){hash^=ch;hash*=1099511628211ull;}std::ostringstream out;out<<std::hex<<std::setw(16)<<std::setfill('0')<<hash;return out.str();}
std::string Catalog::item(const std::string& authored) const {
    auto direct=goods.find(authored);if(direct!=goods.end())return direct->first;
    const auto key=normalized(authored);for(const auto& [name,good]:goods){(void)good;if(normalized(name)==key)return name;}
    throw std::runtime_error("Unknown cargo item: "+authored);
}
Catalog Catalog::load(const std::filesystem::path& root){
    Catalog result;
    for(const auto& row:read_json(root/"Cargo/cargo.json")){
        Good good;good.id=row.at("Name");good.tonnes=row.at("Size").get<double>();require(std::isfinite(good.tonnes)&&good.tonnes>=0,"Invalid cargo mass");good.credits=row.at("Value");good.illicit=row.value("bIllicit",false);good.whole=row.value("Unit",std::string("t"))!="t";good.preview_quantity=row.value("PreviewQuantity",0.0);
        const std::map<std::string,int> rarities={{"Common",0},{"Uncommon",1},{"Rare",2},{"Exotic",3},{"Legendary",4}};const auto rarity=rarities.find(row.value("Rarity",std::string("Common")));good.rarity=rarity==rarities.end()?0:rarity->second;
        require(!good.id.empty()&&good.credits>=0,"Invalid cargo row");require(result.goods.emplace(good.id,good).second,"Duplicate cargo item");
    }
    for(const auto& row:read_json(root/"Cargo/recipes.json")){
        Recipe recipe;recipe.id=row.at("Name");recipe.activity=row.value("Activity",std::string("Manufacturing"));recipe.hull=row.value("ShipDesign",std::string("None"));recipe.input_tonnes=positive(row.at("InputTonnes"),"Recipe input");recipe.seconds=positive(row.at("SecondsPerBatch"),"Batch duration");
        for(const auto& material:row.value("Minerals",json::array()))recipe.minerals.push_back(result.item(material));
        for(const auto& material:row.value("Inputs",json::array()))recipe.inputs.push_back({result.item(material.at("Item")),positive(material.at("Quantity"),"Input quantity")});
        if(row.contains("Outputs"))for(const auto& output:row.at("Outputs")){
            if(recipe.hull!="None" && recipe.hull!="")continue;
            recipe.outputs.push_back({result.item(output.at("Item")),positive(output.at("Quantity"),"Output quantity")});
        }
        if(recipe.outputs.empty() && (recipe.hull=="None"||recipe.hull.empty()))recipe.outputs.push_back({result.item(row.at("CargoItem")),positive(row.value("OutputQuantity",row.value("OutputTonnes",1.0)),"Legacy output")});
        require(result.recipes.emplace(recipe.id,recipe).second,"Duplicate recipe");
    }
    for(const auto& row:read_json(root/"Production/facilities.json")){
        Facility facility;facility.id=row.at("Name");facility.name=row.at("Facility");facility.system=row.at("System");facility.region=row.at("Region");facility.recipe=row.at("Recipe");facility.capacity=positive(row.at("CapacityTonnes"),"Facility capacity");facility.lanes=row.value("ProductionLanes",1);
        require(facility.lanes>=1&&facility.lanes<=16,"Invalid production lane count");
        facility.activities=row.value("AllowedActivities",std::vector<std::string>{"Refining","Manufacturing"});facility.recipes=row.value("AdditionalRecipes",std::vector<std::string>{});facility.recipes.push_back(facility.recipe);
        require(result.recipes.contains(facility.recipe),"Facility references unknown recipe");require(result.facilities.emplace(facility.id,facility).second,"Duplicate facility");
    }
    for(const auto& row:read_json(root/"Cargo/background_mining.json")){
        Mine mine;mine.id=row.at("Id");mine.system=row.at("System");mine.region=row.at("Region");mine.item=result.item(row.at("Item"));mine.reserve=positive(row.at("ReserveTonnes"),"Deposit reserve");mine.rate=positive(row.at("RateTonnesPerSecond"),"Mining rate");mine.capacity=positive(row.at("StorageTonnes"),"Mining storage");result.mines.push_back(mine);
    }
    const auto galaxy=read_json(root/"Galaxy/galaxy.json");
    for(const auto& system:galaxy.at("systems"))result.links[system.at("name")]=system.value("link",std::vector<std::string>{});
    result.markets=read_json(root/"Cargo/economy.json");const auto fees=read_json(root/"Cargo/transaction_fees.json");result.gst_basis_points=fees.value("GstBasisPoints",200);result.service_basis_points=fees.value("ServiceFeeBasisPoints",100);
    require(result.gst_basis_points>=0&&result.gst_basis_points<=10000&&result.service_basis_points>=0&&result.service_basis_points<=10000,"Invalid transaction fees");
    return result;
}
std::vector<std::string> Catalog::route(const std::string& from,const std::string& to) const {
    if(!links.contains(from)||!links.contains(to))return {};
    std::queue<std::string> pending;std::map<std::string,std::string> parent;pending.push(from);parent[from]="";
    while(!pending.empty()){
        auto node=pending.front();pending.pop();if(node==to){std::vector<std::string> path;for(auto current=to;!current.empty();current=parent.at(current))path.push_back(current);std::reverse(path.begin(),path.end());return path;}
        for(const auto& neighbor:links.at(node))if(links.contains(neighbor)&&!parent.contains(neighbor)){parent[neighbor]=node;pending.push(neighbor);}
    }
    return {};
}
World::World(Catalog catalog):catalog_(std::move(catalog)){initialize();}
void World::inventory(const std::string& key,const std::string& owner,const std::string& system,double capacity){
    if(state_["inventories"].contains(key))return;
    state_["inventories"][key]={{"owner",owner},{"system",system},{"capacity",capacity},{"active",true},{"reservedOutput",0.0},{"reservedPickup",json::object()},{"stock",json::object()}};
    if(!state_["wallets"].contains(owner))state_["wallets"][owner]=10000000;
}
void World::initialize(){
    state_={{"schema",1},{"npcAuthorityActive",false},{"worldId",""},{"revision",0},{"clock",0.0},{"sequence",0},{"dispatchCursor",0},{"nextPulse",60.0},{"nextMarket",60.0},{"nextDispatch",60.0},{"completedBatches",0},{"deliveredFreight",0},{"inventories",json::object()},{"wallets",json::object()},{"mines",json::object()},{"jobs",json::array()},{"journeys",json::array()},{"receipts",json::object()},{"history",json::object()},{"events",json::array()},{"hullStock",json::array()}};
    for(const auto& mine:catalog_.mines){const auto hold=mine.system+"/"+mine.region+"/Ore";inventory(hold,"NPC/"+hold,mine.system,mine.capacity);state_["mines"][mine.id]={{"remaining",mine.reserve},{"inventory",hold}};}
    for(const auto& [key,facility]:catalog_.facilities){(void)key;inventory(warehouse(facility,false),"NPC/"+facility.id,facility.system,facility.capacity);inventory(warehouse(facility,true),"NPC/"+facility.id,facility.system,facility.capacity);}
    std::map<std::string,std::string> exclusive;
    for(const auto& market:catalog_.markets)for(const auto& item:market.value("ExclusiveSupply",json::array()))exclusive[catalog_.item(item)]=market.at("Name");
    for(const auto& market:catalog_.markets){
        const std::string system=market.at("Name"),hold=system+"/NPC Market/Stock";inventory(hold,"NPC/"+hold,system,1.e12);
        std::set<std::string> products;for(const auto& item:market.value("Production",json::array()))products.insert(catalog_.item(item));for(const auto& [item,source]:exclusive)if(source==system)products.insert(item);
        for(const auto& item:products){const auto& good=catalog_.goods.at(item);if(good.tonnes<=0)continue;if(exclusive.contains(item)&&exclusive.at(item)!=system)continue;if(good.illicit&&!market.value("bIllicitTrade",false)&&!exclusive.contains(item))continue;const double seed=std::max(1.0,std::min(good.preview_quantity,double(160>>good.rarity)))*2;
            // Same finite starter-stock policy as SSW: not an inexhaustible restock.
            deposit(hold,item,seed);
        }
    }
    validate();
}
std::string World::id(const std::string& prefix){auto number=state_["sequence"].get<std::uint64_t>()+1;state_["sequence"]=number;return prefix+std::to_string(number);}
double World::quantity(const std::string& hold,const std::string& item) const {if(!state_["inventories"].contains(hold))return 0;return state_["inventories"][hold]["stock"].value(item,0.0);}
double World::available(const std::string& hold,const std::string& item) const {return quantity(hold,item)-state_["inventories"][hold]["reservedPickup"].value(item,0.0);}
double World::used(const std::string& hold) const {double total=0;for(const auto& [item,amount]:state_["inventories"][hold]["stock"].items())total+=amount.get<double>()*catalog_.goods.at(item).tonnes;return total;}
void World::deposit(const std::string& hold,const std::string& item,double amount){positive(amount,"Deposit");require(state_["inventories"].contains(hold),"Unknown destination");require(used(hold)+amount*catalog_.goods.at(item).tonnes<=state_["inventories"][hold]["capacity"].get<double>()+epsilon,"Storage capacity exceeded");state_["inventories"][hold]["stock"][item]=quantity(hold,item)+amount;}
bool World::take(const std::string& hold,const std::string& item,double amount){if(!std::isfinite(amount)||amount<=0||available(hold,item)+epsilon<amount)return false;state_["inventories"][hold]["stock"][item]=std::max(0.0,quantity(hold,item)-amount);return true;}
double World::next_deadline() const {double next=std::min({state_["nextPulse"].get<double>(),state_["nextMarket"].get<double>(),state_["nextDispatch"].get<double>()});for(const auto& job:state_["jobs"])if(job["status"]=="Running")next=std::min(next,job["due"].get<double>());for(const auto& trip:state_["journeys"])if(trip["status"]=="Pickup"||trip["status"]=="Transit")next=std::min(next,trip["due"].get<double>());return next;}
void World::advance(double seconds){require(std::isfinite(seconds)&&seconds>=0&&seconds<=86400,"Advance is outside the one-day catch-up budget");const double target=state_["clock"].get<double>()+seconds;std::size_t events=0;while(next_deadline()<=target){require(++events<=200000,"Event catch-up budget exceeded");const double at=next_deadline();state_["clock"]=at;step(at);}state_["clock"]=target;validate();}
void World::step(double now){
    if(now+epsilon>=state_["nextPulse"].get<double>()){
        for(const auto& mine:catalog_.mines){auto& data=state_["mines"][mine.id];const std::string hold=data["inventory"];auto& inv=state_["inventories"][hold];if(!inv["active"].get<bool>())continue;const double count=std::min({data["remaining"].get<double>()/catalog_.goods.at(mine.item).tonnes,mine.rate*60/catalog_.goods.at(mine.item).tonnes,std::max(0.0,(inv["capacity"].get<double>()-used(hold)-inv["reservedOutput"].get<double>())/catalog_.goods.at(mine.item).tonnes)});if(count>epsilon){deposit(hold,mine.item,count);data["remaining"]=std::max(0.0,data["remaining"].get<double>()-count*catalog_.goods.at(mine.item).tonnes);}}
        state_["nextPulse"]=now+60;
    }
    expire_external_trades(state_,catalog_);freight(now);production(now);
    if(now+epsilon>=state_["nextMarket"].get<double>()){sample_market(now);state_["nextMarket"]=now+60;}
    for(const char* key:{"jobs","journeys"})if(state_[key].size()>2000){
        auto& records=state_[key];std::size_t excess=records.size()-2000;
        records.erase(std::remove_if(records.begin(),records.end(),[&](const json& record){const std::string status=record.at("status");if(excess>0&&status!="Running"&&status!="Pickup"&&status!="Transit"&&status!="Stranded"){--excess;return true;}return false;}),records.end());
    }
    state_["revision"]=state_["revision"].get<std::uint64_t>()+1;
}
void World::production(double now){
    for(auto& job:state_["jobs"]){if(job["status"]!="Running"||job["due"].get<double>()>now+epsilon)continue;const auto& facility=catalog_.facilities.at(job["facility"].get<std::string>());const auto output=warehouse(facility,true);auto& inv=state_["inventories"][output];inv["reservedOutput"]=std::max(0.0,inv["reservedOutput"].get<double>()-job["reservedMass"].get<double>());
        if(!inv["active"].get<bool>()){job["status"]="Cancelled";job["reason"]="Facility unavailable; started materials remain consumed";continue;}
        for(const auto& product:job["products"])deposit(output,product.at("item"),product.at("quantity"));
        if(job.contains("hull")&&job["hull"]!="None")state_["hullStock"].push_back({{"id",id("Hull/")},{"design",job["hull"]},{"facility",facility.id}});
        job["status"]="Completed";state_["completedBatches"]=state_["completedBatches"].get<std::size_t>()+1;state_["events"].push_back({{"type","ProductionCompleted"},{"job",job["id"]},{"facility",facility.id},{"at",now}});
    }
    std::map<std::string,int> lane_occupancy;
    for(const auto& job:state_["jobs"])if(job["status"]=="Running")++lane_occupancy[job["facility"].get<std::string>()];
    for(const auto& [key,facility]:catalog_.facilities){
        const auto input=warehouse(facility,false),output=warehouse(facility,true);if(!state_["inventories"][input]["active"].get<bool>()||!state_["inventories"][output]["active"].get<bool>())continue;
        int running=lane_occupancy[key];
        const auto& recipe=catalog_.recipes.at(facility.recipe);if(!contains(facility.activities,recipe.activity))continue;
        while(running<facility.lanes){
            std::vector<Amount> materials=recipe.inputs;bool enough=true;
            if(materials.empty()){double needed=recipe.input_tonnes;for(const auto& mineral:recipe.minerals){const double count=std::min(available(input,mineral),needed/catalog_.goods.at(mineral).tonnes);if(count>epsilon){materials.push_back({mineral,count});needed-=count*catalog_.goods.at(mineral).tonnes;}}enough=needed<epsilon;}
            for(const auto& material:materials)enough=enough&&available(input,material.item)+epsilon>=material.quantity;
            double mass=0;for(const auto& product:recipe.outputs)mass+=product.quantity*catalog_.goods.at(product.item).tonnes;
            auto& inv=state_["inventories"][output];if(!enough||used(output)+inv["reservedOutput"].get<double>()+mass>facility.capacity+epsilon)break;
            json products=json::array(),consumed=json::array();for(const auto& material:materials){require(take(input,material.item,material.quantity),"Atomic material debit failed");consumed.push_back({{"item",material.item},{"quantity",material.quantity}});}for(const auto& product:recipe.outputs)products.push_back({{"item",product.item},{"quantity",product.quantity}});
            inv["reservedOutput"]=inv["reservedOutput"].get<double>()+mass;
            state_["jobs"].push_back({{"id",id("Job/")},{"facility",key},{"recipe",recipe.id},{"status","Running"},{"due",now+recipe.seconds},{"products",products},{"materials",consumed},{"reservedMass",mass},{"hull",recipe.hull}});++running;
        }
    }
    if(state_["events"].size()>1000)state_["events"].erase(state_["events"].begin(),state_["events"].end()-1000);
}
void World::freight(double now){
    for(auto& trip:state_["journeys"]){
        const std::string status=trip["status"];if((status!="Pickup"&&status!="Transit")||trip["due"].get<double>()>now+epsilon)continue;
        const std::string source=trip["source"],destination=trip["destination"],item=trip["item"],buyer=trip["buyer"],seller=trip["seller"];const double count=trip["quantity"];const auto price=trip["credits"].get<std::int64_t>();
        auto& src=state_["inventories"][source];auto& dst=state_["inventories"][destination];
        if(status=="Pickup"){
            src["reservedPickup"][item]=std::max(0.0,src["reservedPickup"].value(item,0.0)-count);
            if(!src["active"].get<bool>()||!take(source,item,count)){
                dst["reservedOutput"]=std::max(0.0,dst["reservedOutput"].get<double>()-count*catalog_.goods.at(item).tonnes);state_["wallets"][buyer]=state_["wallets"][buyer].get<std::int64_t>()+price;trip["status"]="Cancelled";continue;
            }
            trip["status"]="Transit";trip["due"]=now+90.0*std::max<std::size_t>(1,trip["route"].size()-1)+count/3.0;continue;
        }
        dst["reservedOutput"]=std::max(0.0,dst["reservedOutput"].get<double>()-count*catalog_.goods.at(item).tonnes);
        if(!dst["active"].get<bool>()){
            if(used(source)+count*catalog_.goods.at(item).tonnes<=src["capacity"].get<double>()+epsilon){deposit(source,item,count);trip["status"]="Returned";}else trip["status"]="Stranded";
            state_["wallets"][buyer]=state_["wallets"][buyer].get<std::int64_t>()+price;continue;
        }
        deposit(destination,item,count);state_["wallets"][seller]=state_["wallets"][seller].get<std::int64_t>()+price;trip["status"]="Delivered";state_["deliveredFreight"]=state_["deliveredFreight"].get<std::size_t>()+1;
        state_["events"].push_back({{"type","FreightUnloaded"},{"journey",trip["id"]},{"destination",destination},{"item",item},{"quantity",count},{"at",now}});
    }
    if(now+epsilon<state_["nextDispatch"].get<double>())return;
    state_["nextDispatch"]=now+60;
    // A bounded rotating source scan prevents empty early sectors starving distant mines.
    std::vector<std::string> sources;for(const auto& [hold,inv]:state_["inventories"].items())if(inv["active"].get<bool>()&&!inv["stock"].empty()&&(hold.ends_with("/Ore")||hold.ends_with("/Output")))sources.push_back(hold);
    if(sources.empty())return;auto cursor=state_["dispatchCursor"].get<std::size_t>();const auto budget=std::min<std::size_t>(32,sources.size());
    for(std::size_t scan=0;scan<budget;++scan){const auto source=sources[(cursor+scan)%sources.size()];const auto& src=state_["inventories"][source];bool scheduled=false;
        for(const auto& [item,value]:src["stock"].items()){
            (void)value;const auto& good=catalog_.goods.at(item);if(good.tonnes<=0||available(source,item)<epsilon)continue;
            std::vector<std::pair<std::size_t,std::string>> targets;
            for(const auto& [key,facility]:catalog_.facilities){(void)key;const auto destination=warehouse(facility,false);if(!state_["inventories"][destination]["active"].get<bool>())continue;const auto& recipe=catalog_.recipes.at(facility.recipe);bool accepts=contains(recipe.minerals,item);for(const auto& required:recipe.inputs)accepts=accepts||required.item==item;if(!accepts||destination==source)continue;const auto route=catalog_.route(src["system"],facility.system);if(!route.empty())targets.emplace_back(route.size(),destination);}
            if(source.ends_with("/Output")){const std::string market=src["system"].get<std::string>()+"/NPC Market/Stock";if(state_["inventories"].contains(market))targets.emplace_back(1,market);}
            std::sort(targets.begin(),targets.end());
            for(const auto& [hops,destination]:targets){auto& dst=state_["inventories"][destination];const double room=(dst["capacity"].get<double>()-used(destination)-dst["reservedOutput"].get<double>())/good.tonnes;const double desired=destination.ends_with("/Input")?100.0:320.0;double count=std::min({available(source,item),room,std::max(0.0,desired-quantity(destination,item)-dst["reservedOutput"].get<double>()/good.tonnes),48.0/good.tonnes});if(good.whole)count=std::floor(count);if(count<epsilon)continue;
                const auto ask=credits(double(good.credits)*factor(available(source,item),1));const auto bid=credits(double(good.credits)*factor(quantity(destination,item),desired));if(ask>bid)continue;const auto total=credits(count*double(ask));const std::string buyer=dst["owner"],seller=src["owner"];if(state_["wallets"][buyer].get<std::int64_t>()<total)continue;
                state_["wallets"][buyer]=state_["wallets"][buyer].get<std::int64_t>()-total;state_["inventories"][source]["reservedPickup"][item]=state_["inventories"][source]["reservedPickup"].value(item,0.0)+count;dst["reservedOutput"]=dst["reservedOutput"].get<double>()+count*good.tonnes;
                state_["journeys"].push_back({{"id",id("Freight/")},{"source",source},{"destination",destination},{"buyer",buyer},{"seller",seller},{"item",item},{"quantity",count},{"credits",total},{"status","Pickup"},{"route",catalog_.route(src["system"],dst["system"])},{"due",now+count/.75}});scheduled=true;(void)hops;break;
            }
            if(scheduled)break;
        }
    }
    state_["dispatchCursor"]=(cursor+budget)%sources.size();
}
void World::sample_market(double now){for(const auto& [hold,inventory]:state_["inventories"].items())if(hold.ends_with("/NPC Market/Stock"))for(const auto& [item,count]:inventory["stock"].items()){
    const auto key=inventory["system"].get<std::string>()+"/"+item;auto& series=state_["history"][key];if(series.is_null())series=json::array();series.push_back({{"at",now},{"supply",count},{"price",double(catalog_.goods.at(item).credits)*factor(count,80)}});if(series.size()>288)series.erase(series.begin(),series.end()-288);
}}
json World::execute(const json& command){
    const std::string action=command.at("action");
    if(action=="bootstrap_npc")return bootstrap(command);
    if(command.contains("worldId"))require(command.at("worldId")==state_.value("worldId",std::string()),"Economy world identity changed; refusing cross-world replay");
    if(action=="quote_external_trade")return quote_external_trade(state_,catalog_,command);
    if(action=="external_trade_status"){
        const auto& trade=state_.at("externalTrades").at(command.at("tradeId").get<std::string>());
        require(trade.at("actor")==command.at("actor"),"Trade belongs to another player");
        auto current=trade;current["ok"]=true;return current;
    }
    const std::string receipt=command.at("id");require(!receipt.empty()&&receipt.size()<=128,"Invalid command id");
    if(state_["receipts"].contains(receipt)){
        require(state_.contains("commandRequests")&&state_["commandRequests"].contains(receipt)&&state_["commandRequests"][receipt]==command,"Request id belongs to an older or different command");
        return state_["receipts"][receipt];
    }
    if(command.contains("expectedRevision"))require(command.at("expectedRevision")==state_["revision"],"Stale economy revision");
    const auto before=state_;try{
        json reply={{"ok",true},{"id",receipt},{"action",action},{"worldId",state_.value("worldId",std::string())}};
        if(action=="prepare_external_trade"||action=="settle_external_trade"||action=="cancel_external_trade"||action=="external_trade_status"){
            const auto result=external_trade(state_,catalog_,command);for(const auto& [key,value]:result.items())reply[key]=value;
        }else if(action=="create_account"){
            const std::string actor=command.at("actor");require(!actor.empty()&&actor.size()<=128&&actor.starts_with("Pilot/"),"Invalid pilot account");require(!state_["wallets"].contains(actor),"Account already exists");state_["wallets"][actor]=5000;inventory(actor+"/Cargo",actor,command.at("system"),positive(command.value("capacityTonnes",48.0),"Cargo capacity"));
        }else if(action=="buy"){
            const std::string actor=command.at("actor"),source=command.at("market"),destination=actor+"/Cargo",item=catalog_.item(command.at("item"));const double count=positive(command.at("quantity"),"Purchase quantity");require(count<=1.e6,"Purchase quantity too large");require(source.ends_with("/NPC Market/Stock")&&state_["inventories"].contains(source)&&state_["inventories"].contains(destination),"Unknown trade endpoint");auto& src=state_["inventories"][source];auto& dst=state_["inventories"][destination];require(src["active"].get<bool>()&&dst["active"].get<bool>()&&src["system"]==dst["system"],"Trade requires an operational local market");require(dst["owner"]==actor,"Inventory owner mismatch");const auto& good=catalog_.goods.at(item);require(good.tonnes>0,"Passengers require a separate berth manifest");require(!good.whole||std::floor(count)==count,"Whole-unit item requires an integer amount");const auto unit=credits(double(good.credits)*factor(available(source,item),80));const auto total=credits(double(unit)*count);const std::int64_t denominator=10000+catalog_.gst_basis_points;const auto fees=(total/denominator)*catalog_.gst_basis_points+((total%denominator)*catalog_.gst_basis_points)/denominator;const auto base=total-fees;
            require(state_["wallets"].contains(actor)&&state_["wallets"][actor].get<std::int64_t>()>=total,"Insufficient credits");require(used(destination)+dst["reservedOutput"].get<double>()+count*good.tonnes<=dst["capacity"].get<double>()+epsilon,"Cargo hold full");require(take(source,item,count),"Market stock unavailable");deposit(destination,item,count);state_["wallets"][actor]=state_["wallets"][actor].get<std::int64_t>()-total;const std::string seller=src["owner"];state_["wallets"][seller]=state_["wallets"][seller].get<std::int64_t>()+base;state_["wallets"]["Treasury"]=state_["wallets"].value("Treasury",std::int64_t{0})+fees;reply["baseCredits"]=base;reply["feesCredits"]=fees;reply["totalCredits"]=total;
        }else if(action=="set_facility_operational"){
            const auto& facility=catalog_.facilities.at(command.at("facility").get<std::string>());const bool active=command.at("operational");state_["inventories"][warehouse(facility,false)]["active"]=active;state_["inventories"][warehouse(facility,true)]["active"]=active;
        }else throw std::runtime_error("Unsupported economy command");
        state_["revision"]=state_["revision"].get<std::uint64_t>()+1;reply["revision"]=state_["revision"];state_["receipts"][receipt]=reply;state_["commandRequests"][receipt]=command;validate();return reply;
    }catch(...){state_=before;throw;}
}
json World::status() const {
    std::size_t active=0;for(const auto& job:state_["jobs"])if(job["status"]=="Running")++active;
    return {{"npcAuthorityActive",state_.value("npcAuthorityActive",false)},{"worldId",state_.value("worldId",std::string())},{"protocol",1},{"server","FractalEconomy"},{"online",true},{"revision",state_["revision"]},{"economySeconds",state_["clock"]},{"inventories",state_["inventories"].size()},{"activeJobs",active},{"completedJobs",state_["completedBatches"]},{"deliveredFreight",state_["deliveredFreight"]},{"mines",catalog_.mines.size()},{"facilities",catalog_.facilities.size()},{"recipes",catalog_.recipes.size()},{"goods",catalog_.goods.size()}};
}
void World::validate() const {
    require(state_.at("schema")==1,"Unsupported economy snapshot schema");require(std::isfinite(state_.at("clock").get<double>())&&state_.at("clock").get<double>()>=0,"Invalid economy clock");
    if(state_.value("npcAuthorityActive",false))require(!state_.value("worldId",std::string()).empty(),"Live economy needs a persistent world identity");
    for(const char* deadline:{"nextPulse","nextMarket","nextDispatch"}){
        const double value=state_.at(deadline);require(std::isfinite(value)&&value>=state_.at("clock").get<double>()-epsilon,"Invalid scheduler deadline");
    }
    require(state_.at("jobs").is_array()&&state_.at("journeys").is_array()&&state_.at("receipts").is_object(),"Incomplete economy collections");
    for(const auto& [mine,record]:state_.at("mines").items()){
        (void)mine;const double remaining=record.at("remaining");require(std::isfinite(remaining)&&remaining>=0,"Invalid finite ore reserve");
        require(state_.at("inventories").contains(record.at("inventory").get<std::string>()),"Missing ore warehouse");
    }
    for(const auto& [owner,balance]:state_.at("wallets").items()){(void)owner;require(balance.is_number_integer()&&balance.get<std::int64_t>()>=0,"Negative or invalid wallet");}
    for(const auto& [hold,inventory]:state_.at("inventories").items()){
        const double capacity=inventory.at("capacity"),reserved=inventory.at("reservedOutput");require(std::isfinite(capacity)&&capacity>0&&std::isfinite(reserved)&&reserved>=-epsilon,"Invalid storage capacity/reservation");require(used(hold)+reserved<=capacity+epsilon,"Inventory/output reservations exceed capacity");
        for(const auto& [item,count]:inventory.at("reservedPickup").items()){
            require(catalog_.goods.contains(item)&&count.is_number()&&std::isfinite(count.get<double>())&&count.get<double>()>=-epsilon,"Invalid pickup reservation");
            require(count.get<double>()<=inventory.at("stock").value(item,0.0)+epsilon,"Pickup reservation exceeds inventory stock");
        }
        for(const auto& [item,count]:inventory.at("stock").items()){require(catalog_.goods.contains(item),"Unknown item in checkpoint");require(std::isfinite(count.get<double>())&&count.get<double>()>=-epsilon,"Invalid stock quantity");require(inventory.at("reservedPickup").value(item,0.0)<=count.get<double>()+epsilon,"Pickup reservation exceeds source stock");}
    }
}
void World::restore(json snapshot){auto old=state_;state_=std::move(snapshot);try{validate();}catch(...){state_=std::move(old);throw;}}
void World::checkpoint(const std::filesystem::path& path) const {
    validate();if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
    if(state_.value("npcAuthorityActive",false)){
        // Once live exchanges are possible, restoring an older generation can
        // resurrect spent stock while a player retains an applied receipt.
        // Leave a persistent fence before publishing the first live checkpoint.
        auto live=path;live+=".live";
        if(!std::filesystem::exists(live)){std::ofstream marker(live,std::ios::binary);require(bool(marker),"Cannot persist live authority recovery fence");marker<<state_.at("worldId").get<std::string>();marker.flush();require(bool(marker),"Cannot flush live authority recovery fence");}
        {std::ifstream marker(live,std::ios::binary);std::string identity;std::getline(marker,identity);require(identity==state_.at("worldId").get<std::string>(),"Live recovery fence belongs to another or incomplete world");}
#ifdef _WIN32
        HANDLE fence=CreateFileW(live.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);require(fence!=INVALID_HANDLE_VALUE,"Cannot open live recovery fence");const bool durable=FlushFileBuffers(fence)!=FALSE;CloseHandle(fence);require(durable,"Cannot flush live recovery fence");
#endif
    }
    const auto payload=state_.dump();json envelope={{"schema",1},{"checksum",digest(payload)},{"payload",state_}};
    if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());auto temporary=path;temporary+=".tmp";auto backup=path;backup+=".bak";
    {std::ofstream out(temporary,std::ios::binary|std::ios::trunc);require(bool(out),"Cannot create economy checkpoint");out<<envelope.dump();out.flush();require(bool(out),"Cannot write economy checkpoint");}
#ifdef _WIN32
    HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);require(file!=INVALID_HANDLE_VALUE,"Cannot flush checkpoint");const bool flushed=FlushFileBuffers(file)!=FALSE;CloseHandle(file);require(flushed,"Checkpoint flush failed");
    if(std::filesystem::exists(path))require(ReplaceFileW(path.c_str(),temporary.c_str(),backup.c_str(),REPLACEFILE_IGNORE_MERGE_ERRORS,nullptr,nullptr)!=FALSE,"Atomic checkpoint replacement failed");
    else require(MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE,"Atomic checkpoint creation failed");
#else
    if(std::filesystem::exists(path))std::filesystem::copy_file(path,backup,std::filesystem::copy_options::overwrite_existing);std::filesystem::rename(temporary,path);
#endif
}
bool World::load_checkpoint(const std::filesystem::path& path){
    auto backup=path;backup+=".bak";auto live=path;live+=".live";
    if(!std::filesystem::exists(path)&&!std::filesystem::exists(backup)){require(!std::filesystem::exists(live),"Live checkpoints are missing; refusing to reset persistent stock");return false;}
    for(const auto& candidate:{path,backup})try{if(candidate==backup&&std::filesystem::exists(live))throw std::runtime_error("Live NPC checkpoint needs receipt-aware recovery; refusing older stock");const auto envelope=read_json(candidate);const auto payload=envelope.at("payload");require(candidate!=backup||!payload.value("npcAuthorityActive",false),"Live authority cannot roll back settled receipts");require(envelope.at("schema")==1&&envelope.at("checksum")==digest(payload.dump()),"Checkpoint checksum/schema mismatch");restore(payload);return true;}catch(const std::exception&){}
    throw std::runtime_error("No valid economy checkpoint; refusing to reset persistent stock");
}
}
