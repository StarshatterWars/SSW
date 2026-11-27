import json
import tempfile
import unittest
from pathlib import Path
from audit_reachability import audit

class ReachabilityTests(unittest.TestCase):
    def fixture(self, root, cycle=False, disconnected=False):
        data=root/"Content/GameData"
        def put(name,value):
            p=data/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value),encoding="utf-8")
        put("Galaxy/galaxy.json",{"systems":[{"name":"S","link":[] if disconnected else ["T"],"region":[{"name":"Belt","asteroids":2}]},{"name":"T","link":[],"region":[]}]})
        put("Cargo/cargo.json",[{"Name":n,"Type":"Bulk" if n=="ore" else "FinishedGoods","bIllicit":False} for n in ["ore","a","b"]])
        rows=[{"Name":"A","CargoItem":"a","Inputs":[{"Item":"b" if cycle else "ore"}]},{"Name":"B","CargoItem":"b","Inputs":[{"Item":"a"}]}]
        put("Cargo/recipes.json",rows)
        put("Production/facilities.json",[{"Name":"Works","System":"S","Recipe":"A","AdditionalRecipes":["B"],"AutomatedRecipes":["B"]}])
        put("Cargo/background_mining.json",[{"Id":"S/Belt/ore","System":"S","Region":"Belt","Item":"ore","ReserveTonnes":10,"RateTonnesPerSecond":1,"StorageTonnes":5}])
        put("Cargo/economy.json",[{"Name":"T","Production":["a","b"]}])
        return data
    def test_actual_catalog_has_sources_buyers_and_routes(self):
        r=audit(Path(__file__).resolve().parents[2]);self.assertFalse(r["errors"]);self.assertFalse(r["unreachable"]);self.assertFalse([c for c in r["cycles"] if not c["seeded"]])
    def test_startup_stock_cannot_bootstrap_a_cycle(self):
        with tempfile.TemporaryDirectory() as d:
            self.fixture(Path(d),cycle=True);r=audit(Path(d));self.assertTrue(r["unreachable"]);self.assertEqual(r["cycles"][0]["seeded"],False)
    def test_directed_route_is_required(self):
        with tempfile.TemporaryDirectory() as d:
            self.fixture(Path(d),disconnected=True);r=audit(Path(d));self.assertTrue(all("no directed source-to-buyer route" in row["issues"] for row in r["unreachable"]))
    def test_seeded_cycles_can_be_reachable(self):
        with tempfile.TemporaryDirectory() as d:
            data=self.fixture(Path(d),cycle=True);p=data/"Cargo/recipes.json";rows=json.loads(p.read_text());rows.append({"Name":"Seed","CargoItem":"a","Inputs":[{"Item":"ore"}]});p.write_text(json.dumps(rows));p=data/"Production/facilities.json";sites=json.loads(p.read_text());sites[0]["AdditionalRecipes"].append("Seed");sites[0]["AutomatedRecipes"].append("Seed");p.write_text(json.dumps(sites));r=audit(Path(d));self.assertFalse(r["unreachable"]);self.assertTrue(r["cycles"][0]["seeded"])
    def test_unassigned_and_missing_ore_locations_are_reported(self):
        with tempfile.TemporaryDirectory() as d:
            data=self.fixture(Path(d));p=data/"Production/facilities.json";sites=json.loads(p.read_text());sites[0]["AutomatedRecipes"]=[];p.write_text(json.dumps(sites));p=data/"Cargo/background_mining.json";rows=json.loads(p.read_text());rows[0]["Region"]="Missing";p.write_text(json.dumps(rows));r=audit(Path(d));self.assertTrue(r["errors"]);self.assertIn("b",[x["item"] for x in r["unreachable"]])

if __name__=="__main__":unittest.main()
