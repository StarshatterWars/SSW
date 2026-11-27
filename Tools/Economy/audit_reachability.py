"""Read-only authored economy dependency/route audit. Startup stock is not a renewable source."""
from __future__ import annotations
import argparse, collections, json, math
from pathlib import Path

def key(value):
    return str(value).strip().casefold()

def outputs(recipe):
    return [key(row["Item"]) for row in recipe.get("Outputs", [])] or [key(recipe.get("CargoItem") or recipe.get("OutputDisplayName"))]

def audit(root):
    def load(path):
        return json.loads((root / "Content/GameData" / path).read_text(encoding="utf-8-sig"))
    cargo={key(r["Name"]):r for r in load("Cargo/cargo.json")}
    recipes={key(r["Name"]):r for r in load("Cargo/recipes.json")}
    sites=load("Production/facilities.json"); mines=load("Cargo/background_mining.json"); markets=load("Cargo/economy.json")
    galaxy=load("Galaxy/galaxy.json"); graph={key(s["name"]):{key(n) for n in s["link"]} for s in galaxy["systems"]}
    reach={}
    for start in graph:
        visited={start};pending=[start]
        for node in pending:
            for other in sorted(graph.get(node,())):
                if other not in visited:visited.add(other);pending.append(other)
        reach[start]=visited
    def valid_regions(node):
        result=set()
        if isinstance(node,dict):
            if node.get("asteroids",0)>0:result.add(key(node["name"]))
            for value in node.values():result.update(valid_regions(value))
        elif isinstance(node,list):
            for value in node:result.update(valid_regions(value))
        return result
    def sector_regions(node):
        result=set()
        if isinstance(node,dict):
            result.update(key(r["name"]) for r in node.get("region",[]))
            for value in node.values():result.update(sector_regions(value))
        elif isinstance(node,list):
            for value in node:result.update(sector_regions(value))
        return result
    errors=[];sectors={key(s["name"]):sector_regions(s) for s in galaxy["systems"]}
    for rid,recipe in recipes.items():
        for amount in recipe.get("Inputs",[])+recipe.get("Outputs",[]):
            if key(amount["Item"]) not in cargo:errors.append("Unknown commodity in recipe: "+rid+" / "+amount["Item"])
            if "Quantity" in amount and (not math.isfinite(amount["Quantity"]) or amount["Quantity"]<=0):errors.append("Invalid recipe quantity: "+rid)
            if "UnitTonnes" in amount and key(amount["Item"]) in cargo and "Size" in cargo[key(amount["Item"])]:
                if not math.isclose(amount["UnitTonnes"],cargo[key(amount["Item"])]["Size"],rel_tol=1.e-6,abs_tol=1.e-6):errors.append("Catalog/recipe unit mass mismatch: "+rid)
        if recipe.get("Inputs") and "InputTonnes" in recipe:
            mass=sum(i["Quantity"]*i["UnitTonnes"] for i in recipe["Inputs"])
            if not math.isclose(mass,recipe["InputTonnes"],rel_tol=1.e-6,abs_tol=1.e-6):errors.append("Recipe input mass mismatch: "+rid)
        if "InputTonnes" in recipe and recipe.get("OutputTonnes",0)>recipe["InputTonnes"]+1.e-6:errors.append("Recipe creates output mass: "+rid)
    ore_regions={key(s["name"]):valid_regions(s) for s in galaxy["systems"]}
    available=collections.defaultdict(set)
    for m in mines:
        if key(m["Item"]) not in cargo:errors.append("Unknown mined cargo: "+m["Item"])
        if key(m["Region"]) not in ore_regions.get(key(m["System"]),set()):errors.append("Mine has no authored asteroid region: "+m["Id"])
        if m["ReserveTonnes"]>0 and m["RateTonnesPerSecond"]>0 and m["StorageTonnes"]>0:
            available[key(m["Item"])].add(key(m["System"]))
    permitted=[];automated=[]
    for site in sites:
        system=key(site["System"])
        if "Region" in site and key(site["Region"]) not in sectors.get(system,set()):errors.append("Facility region does not exist in its system: "+site["Name"])
        if not 1<=site.get("ProductionLanes",1)<=16:errors.append("Invalid facility lane count: "+site["Name"])
        if system not in graph:errors.append("Unknown facility system: "+site["Name"])
        for rid in {key(site["Recipe"]),*(key(r) for r in site.get("AdditionalRecipes",[]))}:
            recipe=recipes.get(rid)
            if not recipe:errors.append("Unknown recipe: "+rid);continue
            acts={key(a) for a in site.get("AllowedActivities",["Manufacturing","Refining"])}
            if key(recipe.get("Activity","Manufacturing")) not in acts or site.get("ProductionLanes",1)<1:continue
            permitted.append((system,rid,site))
            active={key(site["Recipe"]),*(key(r) for r in site.get("AutomatedRecipes",[]))}
            if rid in active:automated.append((system,rid,site))
    # Monotone fixed point: AND exact materials, OR refining mineral alternatives.
    def solve(assignments):
        known={i:set(v) for i,v in available.items()};enabled=set()
        def can_supply(item,system):return any(system in reach.get(src,{src}) for src in known.get(item,()))
        changed=True
        while changed:
            changed=False
            for system,rid,site in assignments:
                recipe=recipes[rid];inputs=[key(r["Item"]) for r in recipe.get("Inputs",[])]
                ready=all(can_supply(i,system) for i in inputs) if inputs else any(can_supply(key(i),system) for i in recipe.get("Minerals",[]))
                if not ready:continue
                enabled.add((system,rid,site["Name"]))
                for product in outputs(recipe):
                    if system not in known.setdefault(product,set()):known[product].add(system);changed=True
        return known,enabled
    known,enabled=solve(automated);possible,_=solve(permitted)
    # Detect SCCs, then distinguish seeded cycles from unreachable dependency loops.
    edges=collections.defaultdict(set)
    for recipe in recipes.values():
        for product in outputs(recipe):edges[product].update(key(i["Item"]) for i in recipe.get("Inputs",[]))
    index={};low={};stack=[];on=set();cycles=[]
    def visit(v):
        index[v]=low[v]=len(index);stack.append(v);on.add(v)
        for w in sorted(edges[v]):
            if w not in index:visit(w);low[v]=min(low[v],low[w])
            elif w in on:low[v]=min(low[v],index[w])
        if low[v]==index[v]:
            group=[]
            while True:
                w=stack.pop();on.remove(w);group.append(w)
                if w==v:break
            if len(group)>1 or v in edges[v]:cycles.append({"items":sorted(group),"seeded":all(known.get(i) for i in group)})
    for v in sorted(list(edges)):
        if v not in index:visit(v)
    results=[]
    for item in sorted({p for r in recipes.values() for p in outputs(r)} | {i for i,r in cargo.items() if r["Type"]=="FinishedGoods" or r.get("bCivilianConsumable")}):
        buyers=set()
        for m in markets:
            if item in cargo and (cargo[item]["Type"]=="FinishedGoods" or cargo[item].get("bCivilianConsumable")) and (not cargo[item].get("bIllicit") or m.get("bIllicitTrade") or item in {key(i) for i in m.get("ExclusiveSupply",[])}):buyers.add(key(m["Name"]))
        for system,rid,_ in automated:
            if item in {key(i["Item"]) for i in recipes[rid].get("Inputs",[])}:buyers.add(system)
        if item in cargo and cargo[item].get("Unit")=="hulls":
            buyers={system for system,rid,site in automated if site.get("FacilityType")=="Shipyard" and item in outputs(recipes[rid])}
        sources=known.get(item,set());viable=any(dst in reach.get(src,{src}) for src in sources for dst in buyers)
        reasons=[]
        if not sources:reasons.append("no ongoing finite-reserve source chain")
        if not possible.get(item):reasons.append("no reachable permitted recipe chain")
        if not buyers:reasons.append("no eligible buyer")
        if sources and buyers and not viable:reasons.append("no directed source-to-buyer route")
        results.append({"item":item,"sourceSystems":sorted(sources),"buyerSystems":sorted(buyers),"routeAvailable":viable,"issues":reasons})
    return {"scope":"Authored graph feasibility, not throughput, funds, depletion or live freight completion. Finite ore reserves are initial sources; NPC startup stock is excluded.","systems":len(graph),"recipes":len(recipes),"permittedAssignments":len(permitted),"automatedAssignments":len(automated),"enabledAssignments":len(enabled),"errors":errors,"cycles":cycles,"goods":results,"unreachable":[r for r in results if r["issues"]]}

if __name__=="__main__":
    ap=argparse.ArgumentParser();ap.add_argument("--root",type=Path,default=Path(__file__).resolve().parents[2]);ap.add_argument("--output",type=Path,required=True);args=ap.parse_args()
    report=audit(args.root);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({k:len(v) if isinstance(v,list) else v for k,v in report.items() if k not in ["goods","scope"]}))
    if report["errors"] or report["unreachable"] or any(not c["seeded"] for c in report["cycles"]):raise SystemExit(1)
