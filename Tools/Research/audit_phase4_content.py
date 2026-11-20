"""Offline SSW catalog consistency audit, informed by EVE's public SDE structure.
Does not import EVE content or change SSW balance. Exits nonzero on broken links.
"""
from pathlib import Path
import argparse
import json
import math
import re


def audit(root):
    data = root / "Content/GameData"
    load = lambda name: json.loads((data / name).read_text(encoding="utf-8-sig"))
    normalize = lambda value: re.sub(r"[^a-z0-9]", "", value.lower())
    cargo = load("Cargo/cargo.json")
    recipes = load("Cargo/recipes.json")
    facilities = load("Production/facilities.json")
    blueprints = load("Production/blueprints.json")
    skills = load("Skills/skills.json")
    ships = load("Ships/ships.json")
    errors = []
    def unique(rows, field, label):
        result = {}
        for row in rows:
            key = row[field]
            if key in result:
                errors.append(f"{label}: duplicate {key}")
            result[key] = row
        return result
    items = {normalize(x["Name"]): x for x in cargo}
    batches = unique(recipes, "Name", "Recipe")
    books = unique(blueprints, "Recipe", "Blueprint")
    abilities = unique(skills, "Name", "Skill")
    hulls = {normalize(x["Row"]): x for x in ships}
    for recipe in recipes:
        name = recipe["Name"]
        if name not in books:
            errors.append(f"{name}: missing blueprint")
        if recipe.get("ShipDesign", "None") not in ("", "None") and normalize(recipe["ShipDesign"]) not in hulls:
            errors.append(f"{name}: unknown ship design")
        for material in recipe.get("Inputs", []):
            item = items.get(normalize(material["Item"]))
            if item is None:
                errors.append(f"{name}: unknown material {material['Item']}")
            elif not math.isclose(material["UnitTonnes"], item["Size"], abs_tol=1e-6):
                errors.append(f"{name}: material unit mass differs from cargo catalog")
            if material["Quantity"] <= 0 or material["UnitTonnes"] <= 0:
                errors.append(f"{name}: nonpositive material")
        for skill, level in recipe.get("RequiredSkillLevels", {}).items():
            if skill not in abilities or not 1 <= level <= abilities[skill]["MaximumLevel"]:
                errors.append(f"{name}: invalid skill requirement {skill}")
        if recipe.get("SecondsPerBatch", 0) <= 0:
            errors.append(f"{name}: nonpositive batch time")
    supported = {name for facility in facilities for name in [facility["Recipe"]] + facility.get("AdditionalRecipes", [])}
    for name in batches:
        if name not in supported:
            errors.append(f"{name}: no production facility supports its blueprint job")
    outputs = {normalize(x.get("CargoItem") or x.get("OutputDisplayName") or x.get("Product", "")) for x in recipes}
    for item in cargo:
        if item["Type"] == "FinishedGoods" and normalize(item["Name"]) not in outputs:
            errors.append(f"{item['Name']}: finished good has no recipe output")
    for blueprint in blueprints:
        if blueprint["Recipe"] not in batches or blueprint["CopyRuns"] <= 0:
            errors.append(f"Invalid blueprint {blueprint['Recipe']}")
    for facility in facilities:
        for recipe in [facility["Recipe"]] + facility.get("AdditionalRecipes", []):
            if recipe not in batches:
                errors.append(f"{facility['Name']}: missing recipe {recipe}")
        if facility["CapacityTonnes"] <= 0:
            errors.append(f"{facility['Name']}: invalid capacity")
    visiting, visited = set(), set()
    def visit(name):
        if name in visiting:
            errors.append(f"Skill prerequisite cycle at {name}")
            return
        if name in visited:
            return
        visiting.add(name)
        skill = abilities[name]
        for parent in set(skill.get("Prerequisites", [])) | set(skill.get("RequiredSkillLevels", {})):
            if parent not in abilities:
                errors.append(f"{name}: unknown prerequisite {parent}")
            else:
                visit(parent)
        visiting.remove(name)
        visited.add(name)
    for name in abilities:
        visit(name)
    return {"counts": {"cargo": len(cargo), "recipes": len(recipes), "facilities": len(facilities),
                       "blueprints": len(blueprints), "skills": len(skills)},
            "errors": errors, "passed": not errors,
            "scope": "Reference, unit mass, prerequisite and batch consistency; not a live economy proof."}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = audit(args.root)
    text = json.dumps(report, indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text + "\n", encoding="utf-8")
    print(text)
    raise SystemExit(0 if report["passed"] else 1)
