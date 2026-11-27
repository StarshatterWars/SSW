#include "ssw/economy.hpp"
#include <httplib.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#else
#include <unistd.h>
#endif
namespace {
using clock_type=std::chrono::steady_clock;
std::int64_t utc(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
unsigned long process_id(){
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return static_cast<unsigned long>(getpid());
#endif
}
std::string token(){
    std::array<unsigned char,32> bytes{};
#ifdef _WIN32
    if(BCryptGenRandom(nullptr,bytes.data(),static_cast<ULONG>(bytes.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0)throw std::runtime_error("Could not generate local authority key");
#else
    std::random_device random;for(auto& byte:bytes)byte=static_cast<unsigned char>(random());
#endif
    std::string result;constexpr char hex[]="0123456789abcdef";for(auto byte:bytes){result+=hex[byte>>4];result+=hex[byte&15];}return result;
}
void respond(httplib::Response& response,const ssw::json& body,int code=200){response.status=code;response.set_header("Cache-Control","no-store");response.set_content(body.dump(),"application/json");}
}
int run(int argc,char** argv){
    try{
        std::filesystem::path data,state_directory;int port=17881;bool await_bootstrap=false;
        for(int i=1;i<argc;++i){const std::string arg=argv[i];if((arg=="--data"||arg=="--state"||arg=="--port")&&i+1<argc){const std::string value=argv[++i];if(arg=="--data")data=std::filesystem::path(std::u8string(value.begin(),value.end()));else if(arg=="--state")state_directory=std::filesystem::path(std::u8string(value.begin(),value.end()));else port=std::stoi(value);}else if(arg=="--await-bootstrap"){await_bootstrap=true;}else if(arg=="--version"){std::cout<<"FractalEconomy Alpha 5 protocol 1 (standalone C++20)\n";return 0;}else throw std::runtime_error("Usage: FractalEconomy --data <GameData> --state <save directory> [--port 17881]");}
        if(data.empty()||state_directory.empty()||port<1024||port>65535)throw std::runtime_error("A data directory, separate state directory and valid loopback port are required");
        data=std::filesystem::absolute(data);state_directory=std::filesystem::absolute(state_directory);std::filesystem::create_directories(state_directory);
#ifdef _WIN32
        const std::wstring mutex_name=L"Local\\SSW-FractalEconomy-Authority-v1";
        HANDLE instance=CreateMutexW(nullptr,FALSE,mutex_name.c_str());if(!instance||GetLastError()==ERROR_ALREADY_EXISTS)throw std::runtime_error("An economy authority is already running in this Windows session");
#endif
        const auto checkpoint=state_directory/"economy.json";
        ssw::World world(ssw::Catalog::load(data));const bool restored=world.load_checkpoint(checkpoint);if(world.state().value("worldId",std::string()).empty()){auto identified=world.state();identified["worldId"]=token();world.restore(identified);world.checkpoint(checkpoint);}if(!restored){if(await_bootstrap){auto standby=world.state();standby["npcAuthorityActive"]=false;world.restore(standby);}world.checkpoint(checkpoint);}
        #ifdef _WIN32
        HANDLE writer=nullptr;
        const auto claim_writer=[&]{if(writer)return;SetLastError(ERROR_SUCCESS);HANDLE candidate=CreateMutexW(nullptr,FALSE,L"Local\\SSW-NPCEconomy-Writer-v1");if(!candidate||GetLastError()==ERROR_ALREADY_EXISTS){if(candidate)CloseHandle(candidate);throw std::runtime_error("Another game or service owns NPC advancement; handoff must quiesce it first");}writer=candidate;};
        if(!await_bootstrap||world.state().value("npcAuthorityActive",false))claim_writer();
#endif
        const auto key=token();{std::ofstream secret(state_directory/"authority.key",std::ios::trunc);if(!(secret<<key))throw std::runtime_error("Could not write local authority key");}
        const auto began=clock_type::now();std::mutex mutex;std::condition_variable wake;std::atomic<bool> stopping{false};std::string persistence_error;
        httplib::Server http;http.new_task_queue=[] {return new httplib::ThreadPool(2);};http.set_payload_max_length(16*1024*1024);http.set_read_timeout(5,0);http.set_write_timeout(5,0);http.set_keep_alive_max_count(16);
#ifdef _WIN32
        http.set_socket_options([](socket_t socket){int yes=1;setsockopt(socket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&yes),sizeof(yes));});
#endif
        const auto authorized=[&](const httplib::Request& request,httplib::Response& response){if(request.get_header_value("X-SSW-Authority")!=key&&request.get_header_value("Authorization")!="Bearer "+key){respond(response,{{"ok",false},{"error","Local gameplay-authority key required"}},403);return false;}return true;};
        http.Get("/status",[&](const httplib::Request&,httplib::Response& response){std::lock_guard lock(mutex);auto status=world.status();status["handoffProtocol"]=1;status["processId"]=process_id();status["serverUtc"]=utc();status["uptimeSeconds"]=std::chrono::duration<double>(clock_type::now()-began).count();status["checkpointReady"]=persistence_error.empty();status["persistenceError"]=persistence_error;status["authorityScope"]="NPC inventories, production and transport; player assets remain in player records";respond(response,status);});
        http.Get("/v1/snapshot",[&](const httplib::Request& request,httplib::Response& response){if(!authorized(request,response))return;std::lock_guard lock(mutex);respond(response,world.state());});
        http.Post("/v1/commands",[&](const httplib::Request& request,httplib::Response& response){if(!authorized(request,response))return;try{const auto command=ssw::json::parse(request.body);
            const auto action=command.value("action",std::string());
            if(action=="create_account"||action=="buy"){respond(response,{{"ok",false},{"error","Player wallets and cargo belong to the player domain; use prepared external trades"}},400);return;}
            std::lock_guard lock(mutex);if(action=="external_trade_status"||action=="quote_external_trade"){respond(response,world.execute(command));return;}if(!persistence_error.empty()){respond(response,{{"ok",false},{"error","Economy persistence is unavailable; mutations suspended"}},503);return;}const auto before=world.state();try{
#ifdef _WIN32
            if(action=="bootstrap_npc")claim_writer();
#endif
            auto reply=world.execute(command);try{world.checkpoint(checkpoint);}catch(const std::exception& error){persistence_error=error.what();throw;}respond(response,reply);}catch(...){world.restore(before);
#ifdef _WIN32
            if(!before.value("npcAuthorityActive",false)&&writer&&action=="bootstrap_npc"){CloseHandle(writer);writer=nullptr;}
#endif
            throw;}wake.notify_all();}catch(const std::exception& error){respond(response,{{"ok",false},{"error",error.what()}},400);}});
        http.Post("/shutdown",[&](const httplib::Request& request,httplib::Response& response){if(!authorized(request,response))return;std::lock_guard lock(mutex);try{world.checkpoint(checkpoint);respond(response,{{"ok",true}});stopping=true;wake.notify_all();http.stop();}catch(const std::exception& error){respond(response,{{"ok",false},{"error",error.what()}},503);}});
        // Bind before starting the scheduler: a failed port bind must not advance a second universe.
        if(!http.bind_to_port("127.0.0.1",port))throw std::runtime_error("Could not bind the loopback economy port");
        std::thread scheduler([&]{auto last=clock_type::now(),saved=last;bool was_advancing=false;std::unique_lock lock(mutex);while(!stopping.load()){
            const auto now=clock_type::now();const double elapsed=std::chrono::duration<double>(now-last).count();last=now;
            const bool advancing=persistence_error.empty()&&(!await_bootstrap||world.state().value("npcAuthorityActive",false));
            try{if(advancing){world.advance(was_advancing?std::min(elapsed,86400.0):0.0);if(now-saved>=std::chrono::minutes(5)){world.checkpoint(checkpoint);saved=now;}}}catch(const std::exception& error){persistence_error=error.what();std::cerr<<"Economy suspended: "<<persistence_error<<'\n';}
            was_advancing=advancing;
            const double wait=std::clamp(world.next_deadline()-world.state().at("clock").get<double>(),.05,30.0);wake.wait_for(lock,std::chrono::duration<double>(wait));
        }});
        std::cout<<"FractalEconomy ready on 127.0.0.1:"<<port<<"; restored="<<restored<<"; no Unreal runtime\n"<<std::flush;
        const bool listening=http.listen_after_bind();stopping=true;wake.notify_all();scheduler.join();
        if(!listening)throw std::runtime_error("Economy HTTP service stopped unexpectedly");
#ifdef _WIN32
        if(writer)CloseHandle(writer);CloseHandle(instance);
#endif
        return 0;
    }catch(const std::exception& error){std::cerr<<"FractalEconomy: "<<error.what()<<'\n';return 1;}
}

#ifdef _WIN32
int wmain(int argc,wchar_t** argv){
    // Preserve Unicode install/save paths from the Windows command line.
    std::vector<std::string> arguments;arguments.reserve(argc);
    for(int i=0;i<argc;++i){const auto utf8=std::filesystem::path(argv[i]).u8string();arguments.emplace_back(utf8.begin(),utf8.end());}
    std::vector<char*> pointers;pointers.reserve(argc);for(auto& argument:arguments)pointers.push_back(argument.data());
    return run(argc,pointers.data());
}
#else
int main(int argc,char** argv){return run(argc,argv);}
#endif
