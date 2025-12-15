import re

class ConflictGuardrail:
    def __init__(self):
        self.negation_patterns = [
            "don't have", "do not have", "not a ", "isn't a ",
            "never ", "no ", "none", "wrong", "incorrect",
            "actually ", "but ", "however ", "false"
        ]

    def check_conflict(self, memory_facts, generated_response):
        """
        Checks if the generated response conflicts with memory facts.
        Returns a list of conflicts.
        """
        conflicts = []
        response_lower = generated_response.lower()
        
        for fact in memory_facts:
            fact_lower = fact.lower()
            # Simple negation check: if fact is present but negated in response
            # This is a naive implementation for demonstration
            if fact_lower in response_lower:
                for pattern in self.negation_patterns:
                    if f"{pattern}{fact_lower}" in response_lower or f"{fact_lower} is {pattern}" in response_lower:
                        conflicts.append(f"Contradiction detected: Response negates fact '{fact}'")
        
        return conflicts

    def check_causal_paradox(self, premises, update, response):
        """
        Specific check for the Causal Paradox test case.
        """
        # Logic: If A->B->C, and Not A, then Not C (or at least C is uncertain).
        # If response says C is true/false without acknowledging Not A, it's a failure.
        
        response_lower = response.lower()
        
        # Check if the update (Not A) is acknowledged
        if "dead" not in response_lower and "slain" not in response_lower:
             return ["Failure: Response ignores the update (Red Dragon is dead)."]
             
        # Check if the consequence is correctly derived
        if "sun is safe" in response_lower or "safe" in response_lower:
            # This is acceptable if they reason it out.
            pass
        elif "cannot determine" in response_lower or "impossible" in response_lower:
            # This is what the model did. It's technically "safe" but weak reasoning if the chain is clear.
            # However, if the chain was "Dragon wakes Giant", and Dragon is dead, then Giant doesn't wake, so Sun is NOT eaten.
            # So "Sun is safe" is the correct logical deduction.
            # "Cannot determine" implies it lost the link between Dragon and Giant.
            return ["Weak Reasoning: Model failed to link the death of the Dragon to the safety of the Sun."]
            
        return []
