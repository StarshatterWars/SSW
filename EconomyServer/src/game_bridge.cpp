#include "ssw/game_bridge.hpp"
#include "ssw/external_trade.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
namespace ssw {
namespace {
void need(bool value,const std::string& error){if(!value)throw std::runtime_error(error);}
std::string lower(std::string text){for(auto& c:text)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return text;}
json field(const json& o,const std::string& key,json fallback=nullptr){for(const auto& [k,v]:o.items())if(lower(k)==lower(key))return v;return fallback;}
void put(json& o,const std::string& key,json value){for(auto i=o.begin();i!=o.end();)if(lower(i.key())==lower(key))i=o.erase(i);else ++i;o[key]=std::move(value);}
std::string text(const json& o,const std::string& key,const std::string& fallback=""){auto v=field(o,key);return v.is_string()?v.get<std::string>():fallback;}
double number(const json& o,const std::string& key,double fallback=0){auto v=field(o,key);return v.is_number()?v.get<double>():fallback;}
bool flag(const json& o,const std::string& key,bool fallback=false){auto v=field(o,key);return v.is_boolean()?v.get<bool>():fallback;}
std::int64_t money(const json& v){if(v.is_string()){auto s=v.get<std::string>();need(!s.empty()&&s.size()<=19&&std::all_of(s.begin(),s.end(),[](char c){return c>='0'&&c<='9';}),"Invalid exact credit amount");return std::stoll(s);}need(v.is_number_integer()||(v.is_number_float()&&std::isfinite(v.get<double>())&&v.get<double>()==std::floor(v.get<double>())&&v.get<double>()<=9.e15),"Credit amount is not an exact integer");return v.get<std::int64_t>();}
std::string account(const std::string& key){return "NPC/"+key.substr(0,key.find_last_of('/'));}
std::string uid(const json& state,const std::string& key){const auto world=state.value("worldId",std::string());return digest(world+"/"+key)+digest("SSW/"+world+"/"+key);}
std::string status(const json& row,const std::string& key,const std::vector<std::string>& names){auto value=field(row,key);if(value.is_number_integer()){auto i=value.get<std::size_t>();need(i<names.size(),"Invalid legacy state");return names[i];}return value.is_string()?value.get<std::string>():names.front();}
json amounts(const Catalog& c,const json& rows){json result=json::array();for(const auto& row:rows){const auto item=c.item(text(row,"item"));const double count=number(row,"quantity");need(std::isfinite(count)&&count>0,"Invalid job material quantity");result.push_back({{"item",item},{"quantity",count}});}return result;}
json ue_amounts(const Catalog& c,const json& rows){json result=json::array();for(const auto& row:rows){const auto item=row.at("item").get<std::string>();result.push_back({{"item",item},{"quantity",row.at("quantity")},{"unitTonnes",c.goods.at(item).tonnes},{"bWholeUnits",c.goods.at(item).whole}});}return result;}
Facility facility(const Catalog& c,const std::string& key,const json& row){Facility f;f.id=key;f.name=text(row,"facility");f.system=text(row,"system");f.region=text(row,"region");f.recipe=text(row,"recipe");f.capacity=number(row,"capacityTonnes");f.lanes=static_cast<int>(number(row,"productionLanes",1));f.activities=field(row,"allowedActivities",json::array({"Manufacturing","Refining"})).get<std::vector<std::string>>();f.recipes=field(row,"automatedRecipes",json::array()).get<std::vector<std::string>>();f.recipes.insert(f.recipes.begin(),f.recipe);f.kind=text(row,"facilityType","Factory");f.surface=flag(row,"bSurface");need(!f.name.empty()&&!f.system.empty()&&c.links.contains(f.system)&&f.capacity>0&&std::isfinite(f.capacity)&&f.lanes>=1&&f.lanes<=16,"Invalid ground/facility definition");if(f.kind!="Storage")for(const auto& r:f.recipes)need(c.recipes.contains(r),"Unmapped facility recipe: "+r);return f;}
}
void restore_game_facilities(Catalog& c,const json& state){for(const auto& [key,row]:state.value("gameFacilities",json::object()).items())c.facilities[key]=facility(c,key,row);}
void register_game_sites(Catalog& c,json& state,const json& payload){
    auto& definitions=state["gameFacilities"];if(!definitions.is_object())definitions=json::object();
    auto& archive=state["gameArchive"];if(!archive.is_object())archive=json::object();auto sites=field(archive,"groundFactories",json::object());
    for(const auto& [key,row]:payload.value("facilities",json::object()).items()){
        need(key.starts_with("Ground/"),"Only discovered ground sites can be registered");const auto f=facility(c,key,row);c.facilities[key]=f;definitions[key]=row;
        for(const auto& suffix:(f.kind=="Storage"?std::vector<std::string>{"/Stock"}:std::vector<std::string>{"/Input","/Output"})){
            const auto hold=f.system+"/"+f.name+suffix;auto& inv=state["inventories"][hold];
            if(inv.is_null()){const auto owner=account(hold);inv={{"owner",owner},{"system",f.system},{"capacity",f.capacity},{"active",true},{"reservedOutput",0.0},{"reservedPickup",json::object()},{"stock",json::object()},{"hullStorage",f.kind=="Shipyard"}};if(!state["wallets"].contains(owner))state["wallets"][owner]=10000000;}
        }
    }
    for(const auto& [key,row]:payload.value("sites",json::object()).items()){need(definitions.contains(key),"Ground site has no valid production profile");sites[key]=row;}put(archive,"groundFactories",sites);
    for(const auto& id:payload.value("offline",json::array())){const auto found=c.facilities.find(id.get<std::string>());if(found==c.facilities.end())continue;const auto& f=found->second;for(const auto& suffix:(f.kind=="Storage"?std::vector<std::string>{"/Stock"}:std::vector<std::string>{"/Input","/Output"}))state["inventories"][f.system+"/"+f.name+suffix]["active"]=false;}
}
json import_game(Catalog& c,const json& seed,const json& payload){
    need(payload.value("schema",0)==1&&payload.contains("economy")&&payload.contains("traffic"),"Incomplete UE NPC snapshot");
    auto out=seed;const auto& e=payload.at("economy");const auto& traffic=payload.at("traffic");
    out["gameArchive"]=e;out["gameTrafficArchive"]=traffic;out["gameFacilities"]=json::object();for(const auto& [key,row]:payload.value("facilities",json::object()).items())if(key.starts_with("Ground/"))out["gameFacilities"][key]=row;restore_game_facilities(c,out);
    const double clock=number(e,"economyClockSeconds");need(std::isfinite(clock)&&clock>=0,"Invalid UE economy clock");out["clock"]=clock;out["epochUtc"]=payload.value("utc",0.0)-clock;out["lastConsumption"]=clock;out["incomeHour"]=static_cast<std::int64_t>(std::floor(clock/3600));out["nextPulse"]=clock+60;out["nextMarket"]=clock+60;out["nextDispatch"]=clock+8;
    out["jobs"]=json::array();out["journeys"]=json::array();out["hullStock"]=json::array();out["history"]=json::object();
    for(auto& [key,inv]:out["inventories"].items()){(void)key;inv["stock"]=json::object();inv["reservedPickup"]=json::object();inv["reservedOutput"]=0.0;}
    const auto inventories=field(e,"inventories",json::object());const auto wallets=field(e,"wallets",json::object());
    for(const auto& [key,row]:inventories.items()){
        need(text(row,"owner")=="Civilian industry","Non-NPC inventory in UE handoff");
        const double capacity=number(row,"capacityTonnes"),reserved=number(row,"reservedOutputTonnes");need(std::isfinite(capacity)&&capacity>=0&&std::isfinite(reserved)&&reserved>=0,"Invalid UE inventory capacity");
        auto owner=text(row,"tradingOwner");if(owner.empty()||owner=="None")owner=wallets.contains("Civilian industry")&&money(wallets.at("Civilian industry"))>0?"NPC/LegacyIndustry":account(key);need(owner.starts_with("NPC/"),"Non-NPC business owner in UE inventory");
        auto inv=json{{"owner",owner},{"system",text(row,"system")},{"capacity",std::max(capacity,1.e-9)},{"active",flag(row,"bOperational",true)&&capacity>0},{"reservedOutput",reserved},{"reservedPickup",json::object()},{"stock",json::object()},{"unitMass",json::object()},{"ue",row},{"market",flag(row,"bMarket")},{"supplyOnly",flag(row,"bSupplyOnly")},{"desired",field(row,"desiredStock",json::object())}};
        inv["desired"]=json::object();for(const auto& [item,value]:field(row,"desiredStock",json::object()).items())inv["desired"][c.item(item)]=value;
        for(const auto& stack:field(row,"items",json::array())){const auto item=c.item(text(stack,"type"));const double count=number(stack,"quantity");need(std::isfinite(count)&&count>=0,"Invalid UE stock quantity");const double unit_mass=number(stack,"unitTonnes",c.goods.at(item).tonnes);need(std::isfinite(unit_mass)&&unit_mass>=0,"Invalid source unit mass");inv["unitMass"][item]=unit_mass;inv["stock"][item]=inv["stock"].value(item,0.0)+count;}
        out["inventories"][key]=std::move(inv);
    }
    for(const auto& [key,f]:c.facilities){(void)key;for(const auto& suffix:(f.kind=="Storage"?std::vector<std::string>{"/Stock"}:std::vector<std::string>{"/Input","/Output"})){
        const auto hold=f.system+"/"+f.name+suffix;if(!out["inventories"].contains(hold))out["inventories"][hold]={{"owner",account(hold)},{"system",f.system},{"capacity",f.capacity},{"active",true},{"reservedOutput",0.0},{"reservedPickup",json::object()},{"stock",json::object()}};
    }}
    for(const auto& [id,r]:field(e,"reservations",json::object()).items()){
        (void)id;const std::string hold=text(r,"inventory"),item=c.item(text(r,"item"));need(text(r,"owner")=="Civilian industry"&&out["inventories"].contains(hold),"Unmapped NPC pickup reservation");auto& inv=out["inventories"][hold];inv["reservedPickup"][item]=inv["reservedPickup"].value(item,0.0)+number(r,"quantity");
    }
    if(wallets.contains("Civilian industry"))out["wallets"]["NPC/LegacyIndustry"]=money(wallets.at("Civilian industry"));
    for(const auto& [key,value]:wallets.items())if(key.starts_with("NPC/")||key.starts_with("Residents/"))out["wallets"][key]=money(value);
    for(const auto& [key,inv]:out["inventories"].items()){(void)key;const auto owner=inv.at("owner").get<std::string>();if(!out["wallets"].contains(owner))out["wallets"][owner]=10000000;}
    for(const auto& [id,value]:field(e,"backgroundOreRemaining",json::object()).items())if(out["mines"].contains(id)){const double remaining=value;need(std::isfinite(remaining)&&remaining>=0,"Invalid remaining deposit");out["mines"][id]["remaining"]=remaining;}
    for(auto& [id,m]:out["mines"].items()){m["lastAt"]=number(field(e,"backgroundMineLastCycle",json::object()),id,clock);m["due"]=std::max(clock,number(field(e,"backgroundMineDeadlines",json::object()),id,clock+60));if(m["due"].get<double>()==clock)out["nextPulse"]=clock;}
    for(const auto& [key,row]:field(e,"regionalMarkets",json::object()).items()){
        (void)key;const auto system=text(row,"system"),item=c.item(text(row,"item"));auto& h=out["history"][system+"/"+item];h=json::array();
        for(const auto& sample:field(row,"samples",json::array()))h.push_back({{"at",number(sample,"timestampUtc")-out["epochUtc"].get<double>()},{"price",number(sample,"price")},{"supply",number(sample,"supply")},{"demand",number(sample,"demand")}});
        out["consumption"][system+"/"+item]={{"initial",std::max(1.0,number(row,"initialSupply",1))},{"remainder",number(row,"consumptionRemainder")}};
    }
    for(const auto& row:field(e,"jobs",json::array())){
        need(text(row,"owner")=="Civilian industry","Non-NPC industry job in handoff");auto key=text(row,"facility");if(!c.facilities.contains(key)){for(const auto& [id,f]:c.facilities)if(f.name==key){key=id;break;}}
        need(c.facilities.contains(key)&&c.recipes.contains(text(row,"recipe")),"Unmapped active NPC job: "+key);const auto& r=c.recipes.at(text(row,"recipe"));
        const auto state=status(row,"state",{"Queued","Running","Blocked","Completed","Cancelled"});const auto products=amounts(c,field(row,"products",json::array()));double mass=0;for(const auto& p:products)mass+=p.at("quantity").get<double>()*c.goods.at(p.at("item").get<std::string>()).tonnes;
        out["jobs"].push_back({{"id",text(row,"id")},{"facility",key},{"recipe",r.id},{"status",state},{"due",std::max(clock,number(row,"completionAt",clock+number(row,"remaining")))},{"lane",static_cast<int>(number(row,"lane"))},{"duration",number(row,"duration",r.seconds)},{"products",products},{"materials",amounts(c,field(row,"materials",json::array()))},{"reservedMass",mass},{"hull",r.hull},{"input",text(row,"input")},{"output",text(row,"output")},{"ue",row}});
    }
    for(const auto& row:field(traffic,"journeys",json::array())){
        if(flag(row,"bPatrol")||text(row,"item").empty()||text(row,"item")=="None")continue;
        const auto phase=status(row,"phase",{"Waiting","Loading","Traveling","Unloading","Completed","Lost","Blocked"});if(phase=="Completed"||phase=="Lost")continue;
        const auto item=c.item(text(row,"item"));const std::string source=text(row,"sourceInventory"),destination=text(row,"destinationInventory"),hold=text(row,"hold");
        need(out["inventories"].contains(source)&&out["inventories"].contains(destination)&&out["inventories"].contains(hold),"Unmapped freight inventory");
        const double count=number(row,"quantity");const auto buyer=text(row,"buyer"),seller=text(row,"seller");const auto price=money(field(row,"tradeCredits",0));
        auto id=text(row,"id");auto digits=id;digits.erase(std::remove(digits.begin(),digits.end(),'-'),digits.end());
        const auto escrow="NPC freight escrow/"+digits;
        if(price>0&&!flag(row,"bTradeSettled"))need(wallets.contains(escrow)&&money(wallets.at(escrow))==price,"Freight payment escrow does not match its cargo");
        std::vector<std::string> route;for(const auto& sector:field(row,"route",json::array())){auto s=sector.get<std::string>();s=s.substr(0,s.find('/'));if(route.empty()||route.back()!=s)route.push_back(s);}
        const bool pickup=phase=="Waiting"||phase=="Loading";const auto& src=out["inventories"][source];const auto& dst=out["inventories"][destination];
        out["journeys"].push_back({{"id",id},{"source",source},{"destination",destination},{"hold",hold},{"buyer",price>0?buyer:dst.at("owner").get<std::string>()},{"seller",price>0?seller:src.at("owner").get<std::string>()},{"item",item},{"quantity",count},{"credits",price},{"status",pickup?"Pickup":phase=="Blocked"?"Stranded":"Transit"},{"route",route},{"due",clock+std::max(.01,number(row,"remaining"))},{"ue",row}});
    }
    for(const auto& [id,row]:field(e,"serializedHulls",json::object()).items())out["hullStock"].push_back({{"id",id},{"design",text(row,"design")},{"inventory",text(row,"inventory")},{"ue",row}});
    register_game_sites(c,out,{{"offline",field(e,"offlineFacilities",json::array())}});
    return out;
}
json project_game(const Catalog& c,const json& s){
    auto e=s.value("gameArchive",json::object());auto inventories=json::object(),wallets=json::object(),jobs=json::array(),trips=json::array(),serial=json::object(),remaining=json::object(),reservations=json::object();
    for(const auto& [key,inv]:s.at("inventories").items()){
        auto row=inv.value("ue",json::object());put(row,"id",field(row,"id",uid(s,"Inventory/"+key)));put(row,"owner","Civilian industry");put(row,"tradingOwner",inv.at("owner"));put(row,"system",inv.at("system"));put(row,"capacityTonnes",inv.at("capacity"));put(row,"reservedOutputTonnes",inv.at("reservedOutput"));put(row,"bOperational",inv.at("active"));
        put(row,"bMarket",inv.value("market",key.ends_with("/Output")||key.ends_with("/NPC Market/Stock")));put(row,"bSupplyOnly",inv.value("supplyOnly",!key.ends_with("/NPC Market/Stock")));put(row,"bHullStorage",inv.value("hullStorage",flag(row,"bHullStorage",false)));
        auto items=json::array();for(const auto& [item,count]:inv.at("stock").items())if(count.get<double>()>1.e-9){auto identity=uid(s,key+"/"+item);for(const auto& old:field(row,"items",json::array()))if(c.item(text(old,"type"))==item){identity=text(old,"id",identity);break;}items.push_back({{"id",identity},{"type",item},{"quantity",count},{"unitTonnes",inv.value("unitMass",json::object()).value(item,c.goods.at(item).tonnes)}});}put(row,"items",items);
        for(const auto& [item,count]:inv.at("reservedPickup").items())if(count.get<double>()>1.e-9){const auto id=uid(s,"Reservation/"+key+"/"+item);reservations[id]={{"id",id},{"inventory",key},{"owner","Civilian industry"},{"item",item},{"quantity",count}};}
        auto asks=json::object(),bids=json::object();for(const auto& [item,good]:c.goods){const double stock=inv.at("stock").value(item,0.0)-inv.at("reservedPickup").value(item,0.0);const double target=inv.value("desired",json::object()).value(item,0.0);if(stock<=0&&target<=0)continue;const auto price=external_unit_price(s,c,key,item);asks[item]=price;bids[item]=price;}put(row,"askPrices",asks);put(row,"bidPrices",bids);inventories[key]=std::move(row);
    }
    for(const auto& [key,value]:s.at("wallets").items())if(key.starts_with("NPC/")||key.starts_with("Residents/"))wallets[key]=std::to_string(value.get<std::int64_t>());
    for(const auto& job:s.at("jobs")){auto row=job.value("ue",json::object());const auto& f=c.facilities.at(job.at("facility").get<std::string>());put(row,"id",field(row,"id",uid(s,job.at("id"))));put(row,"owner","Civilian industry");put(row,"facility",f.surface?f.id:f.name);put(row,"recipe",job.at("recipe"));put(row,"input",job.value("input",f.system+"/"+f.name+"/Input"));put(row,"output",job.value("output",f.system+"/"+f.name+"/Output"));put(row,"state",job.at("status")=="Paused"?json("Running"):job.at("status"));put(row,"lane",job.value("lane",0));put(row,"materials",ue_amounts(c,job.at("materials")));put(row,"products",ue_amounts(c,job.at("products")));put(row,"duration",job.value("duration",c.recipes.at(job.at("recipe").get<std::string>()).seconds));put(row,"remaining",job.at("status")=="Running"?std::max(0.0,job.at("due").get<double>()-s.at("clock").get<double>()):job.value("remaining",0.0));put(row,"completionAt",job.at("status")=="Paused"?json(0):job.at("due"));jobs.push_back(row);}
    for(const auto& trip:s.at("journeys")){auto row=trip.value("ue",json::object());const auto state=trip.at("status").get<std::string>();put(row,"id",field(row,"id",uid(s,trip.at("id"))));put(row,"nativeId",trip.at("id"));put(row,"phase",state=="Pickup"?"Loading":state=="Transit"?"Traveling":state=="Stranded"?"Blocked":state=="Delivered"||state=="Returned"?"Completed":"Lost");put(row,"sourceInventory",trip.at("source"));put(row,"destinationInventory",trip.at("destination"));put(row,"hold",trip.value("hold",std::string()));put(row,"item",trip.at("item"));put(row,"quantity",trip.at("quantity"));put(row,"unitTonnes",c.goods.at(trip.at("item").get<std::string>()).tonnes);put(row,"remaining",std::max(0.0,trip.at("due").get<double>()-s.at("clock").get<double>()));put(row,"tradeCredits",trip.at("credits"));put(row,"bTradeSettled",state=="Delivered");trips.push_back(row);}
    for(const auto& hull:s.at("hullStock")){auto row=hull.value("ue",json::object());const auto id=text(row,"hullId",uid(s,hull.at("id")));put(row,"hullId",id);put(row,"design",hull.at("design"));auto hold=hull.value("inventory",std::string());if(hold.empty()&&hull.contains("facility")){const auto& f=c.facilities.at(hull.at("facility").get<std::string>());hold=f.system+"/"+f.name+"/Output";}put(row,"inventory",hold);serial[id]=row;}
    for(const auto& [id,m]:s.at("mines").items())remaining[id]=m.at("remaining");
    auto offline=json::array(),policies=json::object();for(const auto& [key,f]:c.facilities){
        const auto hold=f.system+"/"+f.name+(f.kind=="Storage"?"/Stock":"/Input");if(s.at("inventories").contains(hold)&&!s.at("inventories").at(hold).at("active").get<bool>())offline.push_back(key);
        policies[f.surface?f.id:f.name]={{"lanes",f.lanes},{"activities",f.activities}};
    }put(e,"offlineFacilities",offline);put(e,"industryPolicies",policies);
    auto markets=json::object();for(const auto& [key,series]:s.at("history").items()){
        const auto slash=key.find('/');if(slash==std::string::npos)continue;auto samples=json::array();
        for(std::size_t i=series.size()>12?series.size()-12:0;i<series.size();++i){const auto& sample=series[i];samples.push_back({{"timestampUtc",static_cast<std::int64_t>(s.value("epochUtc",0.0)+sample.at("at").get<double>())},{"price",sample.at("price")},{"supply",sample.at("supply")},{"demand",sample.value("demand",0.0)}});}
        markets[key]={{"system",key.substr(0,slash)},{"item",key.substr(slash+1)},{"samples",samples}};
    }put(e,"regionalMarkets",markets);
    put(e,"inventories",inventories);put(e,"wallets",wallets);put(e,"jobs",jobs);put(e,"serializedHulls",serial);put(e,"reservations",reservations);put(e,"backgroundOreRemaining",remaining);put(e,"economyClockSeconds",s.at("clock"));
    return {{"ok",true},{"worldId",s.at("worldId")},{"revision",s.at("revision")},{"economy",e},{"traffic",{{"journeys",trips},{"clock",s.at("clock")}}}};
}
}
