from ai_safety.conflict_guardrail import ConflictGuardrail

guard = ConflictGuardrail()

response = """The facts provided do not contain any information about the safety of the Sun, nor do they mention the Blue Giant or the Red Dragon in 
a context related to the Sun. Therefore, based on the given information, it's impossible to assess the safety of the Sun."""

premises = ["Red Dragon wakes Blue Giant", "Blue Giant eats Sun"]
update = "Red Dragon is dead"

print("Analyzing Response...")
conflicts = guard.check_causal_paradox(premises, update, response)
for c in conflicts:
    print(f"CONFLICT: {c}")

if not conflicts:
    print("No conflicts detected.")
