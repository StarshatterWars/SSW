#pragma once
#include <json.hpp>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace ssw {
using json=nlohmann::json;
struct Good {std::string id;double tonnes=1;std::int64_t credits=0;bool whole=false,illicit=false;double preview_quantity=0;int rarity=0;};
struct Amount {std::string item;double quantity=0;};
struct Recipe {
    std::string id,activity,hull;
    std::vector<Amount> inputs,outputs;
    std::vector<std::string> minerals;
    double input_tonnes=0,seconds=0;
};
struct Facility {std::string id,name,system,region,recipe;double capacity=0;int lanes=1;std::vector<std::string> activities,recipes;};
struct Mine {std::string id,system,region,item;double reserve=0,rate=0,capacity=0;};
struct Catalog {
    std::map<std::string,Good> goods;
    std::map<std::string,Recipe> recipes;
    std::map<std::string,Facility> facilities;
    std::vector<Mine> mines;
    std::map<std::string,std::vector<std::string>> links;
    json markets=json::array();
    int gst_basis_points=200,service_basis_points=100;
    static Catalog load(const std::filesystem::path& root);
    std::string item(const std::string& authored) const;
    std::vector<std::string> route(const std::string& from,const std::string& to) const;
};
class World {
public:
    explicit World(Catalog catalog);
    const Catalog& catalog() const{return catalog_;}
    const json& state() const{return state_;}
    void restore(json snapshot);
    void advance(double seconds);
    json execute(const json& command);
    json status() const;
    json bootstrap(const json& command);
    void validate() const;
    void checkpoint(const std::filesystem::path& file) const;
    bool load_checkpoint(const std::filesystem::path& file);
    double next_deadline() const;
private:
    Catalog catalog_;
    json state_;
    void initialize();
    void step(double now);
    void production(double now);
    void freight(double now);
    void sample_market(double now);
    void inventory(const std::string& id,const std::string& owner,const std::string& system,double capacity);
    double used(const std::string& id) const;
    double quantity(const std::string& id,const std::string& item) const;
    double available(const std::string& id,const std::string& item) const;
    void deposit(const std::string& id,const std::string& item,double amount);
    bool take(const std::string& id,const std::string& item,double amount);
    std::string id(const std::string& prefix);
};
json read_json(const std::filesystem::path& path);
std::string digest(const std::string& text);
}
