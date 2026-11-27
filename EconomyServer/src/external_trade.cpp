#include "ssw/external_trade.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace ssw {
namespace {
constexpr double epsilon=1.e-7;
void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
std::int64_t amount(double value){require(std::isfinite(value)&&value>=0&&value<9.e15,"Trade credit amount outside its safe range");return static_cast<std::int64_t>(std::ceil(value));}
void credit(json& wallets,const std::string& owner,std::int64_t value){const auto old=wallets.value(owner,std::int64_t{0});require(value>=0&&old>=0&&old<=std::numeric_limits<std::int64_t>::max()-value,"Wallet credit overflow");wallets[owner]=old+value;}
double used(const json& inv,const Catalog& catalog){double mass=0;for(const auto& [item,count]:inv.at("stock").items())mass+=count.get<double>()*catalog.goods.at(item).tonnes;return mass;}
json reply(const json& trade){auto result=trade;result["ok"]=true;return result;}
}
json quote_external_trade(const json& world,const Catalog& catalog,const json& command){
    const std::string market=command.at("market"),side=command.at("side"),item=catalog.item(command.at("item"));
    require(side=="Buy"||side=="Sell","Trade side must be Buy or Sell");
    require(world.at("inventories").contains(market),"Unknown NPC inventory");const auto& inv=world.at("inventories").at(market);
    const std::string owner=inv.at("owner");require(owner.starts_with("NPC/")&&inv.at("active").get<bool>(),"An operational NPC market is required");
    const auto& good=catalog.goods.at(item);const double count=command.at("quantity");
    require(good.credits>0&&good.tonnes>0&&std::isfinite(count)&&count>0&&count<=1.e6&&(!good.whole||std::floor(count)==count),"Invalid item quantity or cargo type");
    const double stock=inv.at("stock").value(item,0.0),reserved=inv.at("reservedPickup").value(item,0.0);
    const double factor=std::clamp(.8+80.0/std::max(1.0,stock-reserved),.85,3.0);
    const auto unit=amount(double(good.credits)*factor),total=amount(double(unit)*count);
    const std::int64_t denominator=10000+catalog.gst_basis_points;
    const auto tax=(total/denominator)*catalog.gst_basis_points+((total%denominator)*catalog.gst_basis_points)/denominator;
    return {{"ok",true},{"market",market},{"owner",owner},{"side",side},{"item",item},{"quantity",count},{"unitTonnes",good.tonnes},{"unitPrice",unit},{"totalCredits",total},{"baseCredits",total-tax},{"taxCredits",tax},{"availableQuantity",std::max(0.0,stock-reserved)},{"revision",world.at("revision")}};
}
json external_trade(json& world,const Catalog& catalog,const json& command){
    const std::string action=command.at("action"),actor=command.at("actor");
    require(actor.starts_with("Pilot/")&&actor.size()<=128,"Trusted player principal required");
    if(!world.contains("externalTrades"))world["externalTrades"]=json::object();auto& trades=world["externalTrades"];
    if(action=="prepare_external_trade"){
        const std::string id=command.at("tradeId");
        require(!id.empty()&&id.size()<=128&&!trades.contains(id)&&trades.size()<100000,"Invalid, reused or exhausted trade identifier");
        const auto quote=quote_external_trade(world,catalog,command);
        const std::string market=quote.at("market"),side=quote.at("side"),item=quote.at("item"),owner=quote.at("owner");
        auto& inv=world["inventories"][market];const auto& good=catalog.goods.at(item);const double count=quote.at("quantity"),mass=count*good.tonnes;
        const double stock=inv["stock"].value(item,0.0),reserved=inv["reservedPickup"].value(item,0.0);
        const auto unit=quote.at("unitPrice").get<std::int64_t>(),total=quote.at("totalCredits").get<std::int64_t>(),tax=quote.at("taxCredits").get<std::int64_t>();
        const auto& expected=command.at("expectedTotal");std::int64_t expected_total=-1;
        if(expected.is_number_integer())expected_total=expected.get<std::int64_t>();
        else if(expected.is_string()){const auto text=expected.get<std::string>();require(!text.empty()&&text.size()<=16&&std::all_of(text.begin(),text.end(),[](char c){return c>='0'&&c<='9';}),"Invalid whole-credit quote");expected_total=std::stoll(text);}
        require(expected_total==total,"Price changed; refresh the quote before paying");
        if(side=="Buy"){
            require(stock-reserved+epsilon>=count,"NPC stock is unavailable");inv["reservedPickup"][item]=reserved+count;
        }else{
            require(used(inv,catalog)+inv["reservedOutput"].get<double>()+mass<=inv["capacity"].get<double>()+epsilon,"NPC storage is full");
            const auto balance=world["wallets"].at(owner).get<std::int64_t>();require(balance>=total,"NPC buyer has insufficient budget");
            world["wallets"][owner]=balance-total;inv["reservedOutput"]=inv["reservedOutput"].get<double>()+mass;
        }
        trades[id]={{"worldId",world.value("worldId",std::string())},{"tradeId",id},{"actor",actor},{"market",market},{"owner",owner},{"side",side},{"item",item},{"quantity",count},{"unitTonnes",good.tonnes},{"unitPrice",unit},{"totalCredits",total},{"baseCredits",total-tax},{"taxCredits",tax},{"state","Prepared"},{"expiresAt",world.at("clock").get<double>()+300}};
        return reply(trades[id]);
    }
    const std::string id=command.at("tradeId");require(trades.contains(id),"Unknown trade receipt");auto& trade=trades[id];
    require(trade.at("actor")==actor,"Trade belongs to another player");
    if(action!="cancel_external_trade"&&trade.at("state")=="Prepared"&&trade.value("expiresAt",std::numeric_limits<double>::max())<=world.at("clock").get<double>()){
        auto cancel=command;cancel["action"]="cancel_external_trade";
        return external_trade(world,catalog,cancel);
    }
    if(action=="external_trade_status")return reply(trade);
    require(action=="settle_external_trade"||action=="cancel_external_trade","Unsupported external trade action");
    if(trade.at("state")!="Prepared")return reply(trade); // Terminal receipts never exchange twice.
    auto& inv=world["inventories"][trade.at("market").get<std::string>()];const std::string item=trade.at("item"),owner=trade.at("owner");
    const double count=trade.at("quantity"),mass=count*trade.at("unitTonnes").get<double>();const bool buy=trade.at("side")=="Buy";
    if(action=="cancel_external_trade"){
        if(buy)inv["reservedPickup"][item]=std::max(0.0,inv["reservedPickup"].value(item,0.0)-count);
        else{inv["reservedOutput"]=std::max(0.0,inv["reservedOutput"].get<double>()-mass);credit(world["wallets"],owner,trade.at("totalCredits"));}
        trade["state"]="Cancelled";trade["cancellationReason"]=trade.value("expiresAt",std::numeric_limits<double>::max())<=world.at("clock").get<double>()?"Expired":"Cancelled by player";return reply(trade);
    }
    require(command.at("playerEscrowId")==id,"Durable player escrow reference required before settlement");
    require(inv.at("active").get<bool>(),"NPC inventory unavailable; trade remains prepared for recovery");
    if(buy){
        require(inv["stock"].value(item,0.0)+epsilon>=count&&inv["reservedPickup"].value(item,0.0)+epsilon>=count,"Reserved NPC stock changed");
        inv["stock"][item]=std::max(0.0,inv["stock"].value(item,0.0)-count);inv["reservedPickup"][item]=std::max(0.0,inv["reservedPickup"].value(item,0.0)-count);
        credit(world["wallets"],owner,trade.at("baseCredits"));
    }else{
        require(used(inv,catalog)+count*trade.at("unitTonnes").get<double>()<=inv["capacity"].get<double>()+epsilon,"NPC output capacity changed");
        inv["reservedOutput"]=std::max(0.0,inv["reservedOutput"].get<double>()-mass);inv["stock"][item]=inv["stock"].value(item,0.0)+count;
    }
    credit(world["wallets"],"Treasury",trade.at("taxCredits"));trade["state"]="Committed";
    world["events"].push_back({{"type","ExternalTradeCommitted"},{"tradeId",id},{"actor",actor},{"at",world.at("clock")}});
    return reply(trade);
}
void expire_external_trades(json& world,const Catalog& catalog){
    if(!world.contains("externalTrades"))return;
    for(auto& [id,trade]:world["externalTrades"].items())if(trade.at("state")=="Prepared"&&trade.value("expiresAt",std::numeric_limits<double>::max())<=world.at("clock").get<double>()){
        external_trade(world,catalog,{{"action","cancel_external_trade"},{"tradeId",id},{"actor",trade.at("actor")}});
    }
}
}
