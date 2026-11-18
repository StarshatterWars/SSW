#pragma once
class Mission;
// Loading a cinematic may build visuals, but must not advance its ships/events.
class FPreparedSceneGate
{
public:
    void Prepare(const Mission* Value) { Prepared=Value; }
    void Begin(const Mission* Value) { if(Prepared==Value)Prepared=nullptr; }
    void Reset() { Prepared=nullptr; }
    bool Blocks(const Mission* Current) const { return Prepared && Prepared==Current; }
private:
    const Mission* Prepared=nullptr;
};
