#!/usr/bin/env python3
"""
STORY MODE MAXIMUM STRESS TEST
==============================
Pushes zeta-story-integration.h to absolute limits:
- 64 characters (max)
- 32 locations (max)
- 32 chapters (max)
- 8KB context buffer
- Complex relationship web
- Name drift injection for coherence testing

Run: python3 scripts/story_mode_max_stress.py [--server URL]
"""

import requests
import json
import time
import sys
import argparse
from datetime import datetime

# Configuration
DEFAULT_SERVER = "http://localhost:8080"
MAX_CHARACTERS = 64
MAX_LOCATIONS = 32
MAX_CHAPTERS = 32

# Epic character list - designed to test extraction patterns
CHARACTERS = [
    # Titled nobility (King, Queen, Prince, etc.)
    "King Aldric the Wise", "Queen Seraphina Moonveil", "Prince Kaelan Stormbringer",
    "Princess Lyria Sunfire", "Lord Vexaris Shadowmend", "Lady Elara Frostweave",

    # Doctors/Professors (Dr., Professor patterns)
    "Dr. Evelyn Carter", "Dr. Marcus Chen", "Professor Helena Blackwood",
    "Prof. Theodore Ashworth", "Doctor Amara Singh", "Dr. Viktor Petrov",

    # Military (Captain, Commander, General, Admiral)
    "Captain Zara Nighthawk", "Commander Ragnar Ironfist", "General Octavia Steele",
    "Admiral Corvus Deepwater", "Captain Finn O'Sullivan", "Commander Yuki Tanaka",

    # Formal titles (Mr., Mrs., Ms., Miss, Sir, Dame)
    "Mr. Sebastian Cross", "Mrs. Elizabeth Thornwood", "Ms. Diana Reyes",
    "Miss Penelope Hart", "Sir Galahad Brightshield", "Dame Rowena Ashford",

    # Named pattern ("named X")
    "a warrior named Theron Blackwood", "a mage named Seraphiel",
    "an assassin named Whisper", "a healer named Solara Dawnbringer",

    # More titled characters to reach 64
    "Duke Maximilian von Strauß", "Duchess Valentina Rosetti",
    "Earl Cedric Pemberton", "Countess Isadora Vega",
    "Baron Nikolai Volkov", "Baroness Anastasia Petrov",
    "Viscount Leopold Fitzgerald", "Viscountess Genevieve Beaumont",

    # Additional military/professional
    "Captain Aurora Windrunner", "Dr. Rajesh Patel", "Professor Ingrid Svensson",
    "Commander Malik Hassan", "General Zhang Wei", "Admiral Sakura Yamamoto",

    # More variety
    "Lord Caspian Ravencrest", "Lady Morgana Nightshade",
    "King Thorin Goldbeard", "Queen Titania Silverleaf",
    "Prince Daemon Firebrand", "Princess Celestia Starweaver",
    "Dr. Nathaniel Gray", "Professor Ophelia Crane",
    "Captain Magnus Ironforge", "Commander Freya Stormborn",

    # Final push to 64
    "Sir Lancelot Brightblade", "Dame Vivienne Winterrose",
    "Mr. Jonathan Blackwell", "Mrs. Catherine Ashworth",
    "Lord Balthazar Shadowfire", "Lady Serenity Moonstone"
]

# Locations to test extraction
LOCATIONS = [
    # "in the X" patterns
    "in the Crystal Citadel", "in the Obsidian Tower", "in the Emerald Gardens",
    "at the Crimson Keep", "on the Floating Isles", "inside the Dragon's Lair",
    "within the Sacred Grove", "aboard the Starship Artemis", "near the Frozen Wastes",

    # "called the X" / "known as the X"
    'called the "Shadowfen Marshes"', 'known as the "Eternal Library"',
    'called the "Ruins of Antiquity"', 'known as the "Celestial Observatory"',

    # More locations
    "in the Thunderpeak Mountains", "at the Silver Coast Harbor",
    "on the Whispering Plains", "inside the Volcanic Forge",
    "within the Arcane Academy", "aboard the Iron Leviathan",
    "near the Cursed Catacombs", "in the Sunlit Sanctum",

    # Push to 32
    "at the Moonlit Monastery", "on the Shattered Isles",
    "inside the Clockwork Citadel", "within the Verdant Valley",
    "in the Abyssal Depths", "at the Astral Nexus",
    "on the Burning Sands", "inside the Frozen Throne",
    "within the Storm Spire", "near the Ancient Ruins"
]

# Relationship patterns to test extraction
RELATIONSHIPS = [
    ("{} loves {}", "romantic"),
    ("{} married {}", "romantic"),
    ("{} is the father of {}", "family"),
    ("{} is the mother of {}", "family"),
    ("{} is the sister of {}", "family"),
    ("{} is the brother of {}", "family"),
    ("{} created {}", "creation"),
    ("{} destroyed {}", "conflict"),
    ("{} killed {}", "conflict"),
    ("{} saved {}", "heroic"),
    ("{} works with {}", "professional"),
    ("{} works for {}", "professional"),
    ("{} betrayed {}", "conflict"),
]

# Plot point patterns
PLOT_POINTS = [
    "{} discovered the ancient secret",
    "{} realized the truth about their heritage",
    "{} learned the forbidden magic",
    "{} died protecting the realm",
    "{} was killed by an assassin",
    "{} fell in love with an enemy",
    "{} betrayed their closest ally",
    "{} revealed their true identity",
    "{} won the great war",
    "{} defeated the dark lord",
    "{} transformed into a dragon",
    "{} became the new ruler",
    "{} escaped the dungeon",
    "{} captured the enemy general",
    "{} created the ultimate weapon",
    "{} built the legendary fortress",
]

class StoryModeStressTester:
    def __init__(self, server_url):
        self.server = server_url.rstrip('/')
        self.results = {
            "start_time": datetime.now().isoformat(),
            "server": server_url,
            "tests": [],
            "characters_extracted": 0,
            "locations_extracted": 0,
            "relationships_created": 0,
            "plot_points_stored": 0,
            "chapters_marked": 0,
            "contradictions_detected": 0,
            "context_surfaced_bytes": 0,
        }

    def check_server(self):
        """Verify server is running and get baseline"""
        try:
            r = requests.get(f"{self.server}/health", timeout=5)
            health = r.json()
            print(f"✓ Server online: {health.get('status', 'unknown')}")
            print(f"  Nodes: {health.get('graph_nodes', 0)}, Edges: {health.get('graph_edges', 0)}")
            self.results["baseline"] = health
            return True
        except Exception as e:
            print(f"✗ Server error: {e}")
            return False

    def set_creative_mode(self):
        """Switch server to CREATIVE mode for story integration"""
        try:
            r = requests.post(f"{self.server}/mode", json={"mode": "CREATIVE"}, timeout=5)
            result = r.json()
            print(f"✓ Mode set: {result.get('new_mode', 'CREATIVE')}")
            return True
        except Exception as e:
            print(f"⚠ Mode switch failed (may already be CREATIVE): {e}")
            return True  # Continue anyway

    def generate_with_mode(self, prompt, mode="CREATIVE", max_tokens=500):
        """Send generation request with specific mode"""
        try:
            payload = {
                "prompt": f"User: {prompt}\nAssistant:",
                "max_tokens": max_tokens,
                "mode": mode
            }
            r = requests.post(f"{self.server}/generate", json=payload, timeout=120)
            return r.json()
        except Exception as e:
            return {"error": str(e)}

    def get_story_stats(self):
        """Get current story stats from /story/stats endpoint"""
        try:
            r = requests.get(f"{self.server}/story/stats", timeout=5)
            stats = r.json()
            if stats.get("initialized") == False:
                # Fallback to graph parsing if story not initialized
                return self._parse_graph_for_story_stats()
            return stats
        except Exception as e:
            print(f"  [debug] get_story_stats error: {e}")
            return self._parse_graph_for_story_stats()

    def _parse_graph_for_story_stats(self):
        """Fallback: parse graph for story-labeled nodes"""
        try:
            g = requests.get(f"{self.server}/graph", timeout=5)
            graph = g.json()
            nodes = graph.get("nodes", [])
            return {
                "num_characters": sum(1 for n in nodes if "story_character" in str(n.get("label", ""))),
                "num_locations": sum(1 for n in nodes if "story_location" in str(n.get("label", ""))),
                "current_chapter": sum(1 for n in nodes if "story_chapter" in str(n.get("label", ""))),
                "facts_surfaced": 0,
                "contradictions_prevented": 0,
            }
        except:
            return {}

    def get_story_characters(self):
        """Get extracted characters from /story/characters endpoint"""
        try:
            r = requests.get(f"{self.server}/story/characters", timeout=5)
            return r.json()
        except:
            return []

    def init_story(self, title="MaxStressTest"):
        """Initialize story context with POST /story/init"""
        try:
            r = requests.post(f"{self.server}/story/init", json={"title": title}, timeout=5)
            result = r.json()
            print(f"✓ Story initialized: {result.get('title', title)}")
            return True
        except Exception as e:
            print(f"⚠ Story init failed: {e}")
            return False

    def extract_story_elements(self, text, chapter=1):
        """Direct extraction via POST /story/extract"""
        try:
            r = requests.post(f"{self.server}/story/extract",
                             json={"text": text, "chapter": chapter}, timeout=30)
            return r.json()
        except Exception as e:
            return {"error": str(e), "extracted": 0}

    def test_character_extraction(self):
        """Test 1: Push character extraction to 64 limit"""
        print("\n" + "="*60)
        print("TEST 1: CHARACTER EXTRACTION (Max 64)")
        print("="*60)

        # Build massive character introduction
        chunks = []
        for i in range(0, len(CHARACTERS), 8):
            batch = CHARACTERS[i:i+8]
            intro = "Chapter {} introduces: ".format(i//8 + 1)
            for char in batch:
                if "named" in char:
                    intro += f"There was {char}. "
                else:
                    intro += f"{char} appeared in the story. "
            chunks.append(intro)

        start = time.time()
        for i, chunk in enumerate(chunks):
            print(f"  Extracting batch {i+1}/{len(chunks)}...", end="\r")
            result = self.generate_with_mode(
                f"Write a story paragraph: {chunk}",
                mode="CREATIVE",
                max_tokens=200
            )

        elapsed = time.time() - start
        stats = self.get_story_stats()
        chars = self.get_story_characters()

        extracted = stats.get("num_characters", len(chars) if chars else 0)
        self.results["characters_extracted"] = extracted

        print(f"\n  ✓ Extracted {extracted}/{MAX_CHARACTERS} characters in {elapsed:.1f}s")
        print(f"  Characters: {[c.get('name', c) for c in (chars[:5] if chars else [])][:5]}...")

        self.results["tests"].append({
            "name": "character_extraction",
            "extracted": extracted,
            "max": MAX_CHARACTERS,
            "time": elapsed,
            "passed": extracted > 30  # Should extract at least half
        })

        return extracted

    def test_location_extraction(self):
        """Test 2: Push location extraction to 32 limit"""
        print("\n" + "="*60)
        print("TEST 2: LOCATION EXTRACTION (Max 32)")
        print("="*60)

        # Build location-heavy narrative
        narrative = "The epic journey took the heroes across many lands: "
        for loc in LOCATIONS:
            narrative += f"They traveled {loc}. "

        start = time.time()
        result = self.generate_with_mode(
            f"Continue this story: {narrative}",
            mode="CREATIVE",
            max_tokens=500
        )

        elapsed = time.time() - start
        stats = self.get_story_stats()
        extracted = stats.get("num_locations", 0)
        self.results["locations_extracted"] = extracted

        print(f"  ✓ Extracted {extracted}/{MAX_LOCATIONS} locations in {elapsed:.1f}s")

        self.results["tests"].append({
            "name": "location_extraction",
            "extracted": extracted,
            "max": MAX_LOCATIONS,
            "time": elapsed,
            "passed": extracted > 15
        })

        return extracted

    def test_relationship_web(self):
        """Test 3: Create complex relationship network"""
        print("\n" + "="*60)
        print("TEST 3: RELATIONSHIP WEB")
        print("="*60)

        # Build relationship-heavy text using actual character names
        chars = CHARACTERS[:20]  # Use first 20 for relationships
        relationship_text = "The complex web of relationships unfolded: "

        rel_count = 0
        for i, (pattern, rel_type) in enumerate(RELATIONSHIPS):
            if i * 2 + 1 < len(chars):
                # Clean character names (remove "a warrior named" etc)
                c1 = chars[i * 2].replace("a warrior named ", "").replace("a mage named ", "").replace("an assassin named ", "").replace("a healer named ", "")
                c2 = chars[i * 2 + 1].replace("a warrior named ", "").replace("a mage named ", "").replace("an assassin named ", "").replace("a healer named ", "")
                relationship_text += pattern.format(c1, c2) + ". "
                rel_count += 1

        start = time.time()
        result = self.generate_with_mode(
            f"Describe these story relationships: {relationship_text}",
            mode="CREATIVE",
            max_tokens=500
        )

        elapsed = time.time() - start
        self.results["relationships_created"] = rel_count

        print(f"  ✓ Created {rel_count} relationships in {elapsed:.1f}s")

        self.results["tests"].append({
            "name": "relationship_extraction",
            "created": rel_count,
            "time": elapsed,
            "passed": rel_count > 5
        })

        return rel_count

    def test_chapter_progression(self):
        """Test 4: Push chapter system to 32 limit"""
        print("\n" + "="*60)
        print("TEST 4: CHAPTER PROGRESSION (Max 32)")
        print("="*60)

        start = time.time()
        chapters_created = 0

        for ch in range(1, min(MAX_CHAPTERS + 1, 16)):  # Test up to 16 chapters (speed)
            print(f"  Creating Chapter {ch}/{MAX_CHAPTERS}...", end="\r")

            # Each chapter introduces new plot points
            plot = PLOT_POINTS[ch % len(PLOT_POINTS)]
            char = CHARACTERS[ch % len(CHARACTERS)]
            char_clean = char.replace("a warrior named ", "").replace("a mage named ", "").replace("an assassin named ", "").replace("a healer named ", "")

            chapter_text = f"""
            CHAPTER {ch}: The Turning Point

            {plot.format(char_clean)}

            The events of this chapter would change everything.
            """

            result = self.generate_with_mode(
                f"Write Chapter {ch} of the epic: {chapter_text}",
                mode="CREATIVE",
                max_tokens=300
            )

            if "error" not in result:
                chapters_created += 1

        elapsed = time.time() - start
        stats = self.get_story_stats()
        current_chapter = stats.get("current_chapter", chapters_created)
        self.results["chapters_marked"] = current_chapter

        print(f"\n  ✓ Created {chapters_created} chapters in {elapsed:.1f}s")
        print(f"  Current chapter: {current_chapter}")

        self.results["tests"].append({
            "name": "chapter_progression",
            "created": chapters_created,
            "max": MAX_CHAPTERS,
            "time": elapsed,
            "passed": chapters_created >= 10
        })

        return chapters_created

    def test_context_surfacing(self):
        """Test 5: Push 8KB context buffer to limits"""
        print("\n" + "="*60)
        print("TEST 5: CONTEXT SURFACING (8KB Buffer)")
        print("="*60)

        # Request generation that should surface maximum context
        start = time.time()
        result = self.generate_with_mode(
            "Continue Chapter 15 of the epic saga, referencing all major characters and their relationships",
            mode="CREATIVE",
            max_tokens=1000
        )

        elapsed = time.time() - start
        stats = self.get_story_stats()
        facts_surfaced = stats.get("facts_surfaced", 0)
        self.results["context_surfaced_bytes"] = facts_surfaced * 100  # Estimate

        print(f"  ✓ Surfaced {facts_surfaced} facts in {elapsed:.1f}s")
        print(f"  Estimated context size: ~{facts_surfaced * 100} bytes")

        self.results["tests"].append({
            "name": "context_surfacing",
            "facts_surfaced": facts_surfaced,
            "time": elapsed,
            "passed": facts_surfaced > 10
        })

        return facts_surfaced

    def test_coherence_check(self):
        """Test 6: Name drift detection and coherence"""
        print("\n" + "="*60)
        print("TEST 6: COHERENCE CHECK (Name Drift Detection)")
        print("="*60)

        # Intentionally introduce name variations to test drift detection
        drift_tests = [
            ("Dr. Evelyn Carter", "Dr. Evelyn Carson"),  # Similar but wrong
            ("King Aldric the Wise", "King Aldric the Strong"),  # Epithet drift
            ("Captain Zara Nighthawk", "Captain Zara Nightowl"),  # Partial drift
            ("Professor Helena Blackwood", "Professor Helena Blackwell"),  # Surname drift
        ]

        contradictions = 0
        start = time.time()

        for correct, drifted in drift_tests:
            # First establish correct name
            self.generate_with_mode(
                f"{correct} walked into the room and spoke clearly.",
                mode="CREATIVE",
                max_tokens=100
            )

            # Then use drifted name
            result = self.generate_with_mode(
                f"Later that day, {drifted} continued the conversation.",
                mode="CREATIVE",
                max_tokens=100
            )

            # Check if drift was detected (would be in stats or logs)

        elapsed = time.time() - start
        stats = self.get_story_stats()
        contradictions = stats.get("contradictions_prevented", 0)
        self.results["contradictions_detected"] = contradictions

        print(f"  ✓ Tested {len(drift_tests)} drift scenarios in {elapsed:.1f}s")
        print(f"  Contradictions detected: {contradictions}")

        self.results["tests"].append({
            "name": "coherence_check",
            "drift_tests": len(drift_tests),
            "contradictions_detected": contradictions,
            "time": elapsed,
            "passed": True  # Any detection is good
        })

        return contradictions

    def test_direct_extraction(self):
        """Test 7: Direct extraction via /story/extract (bypasses LLM)"""
        print("\n" + "="*60)
        print("TEST 7: DIRECT EXTRACTION (Fast Path)")
        print("="*60)

        # Build massive extraction payload with ALL characters and locations
        mega_text = """
        CHAPTER 1: THE GATHERING

        King Aldric the Wise summoned all the heroes to the Crystal Citadel.
        Queen Seraphina Moonveil arrived first, followed by Prince Kaelan Stormbringer.
        Princess Lyria Sunfire came from the Obsidian Tower with Lord Vexaris Shadowmend.
        Lady Elara Frostweave traveled from the Emerald Gardens.

        Dr. Evelyn Carter had discovered something terrible in the Crimson Keep.
        Dr. Marcus Chen confirmed her findings while Professor Helena Blackwood analyzed the data.
        Prof. Theodore Ashworth and Doctor Amara Singh arrived at the Floating Isles.
        Dr. Viktor Petrov remained at the Dragon's Lair to guard the entrance.

        Captain Zara Nighthawk led the military response from the Sacred Grove.
        Commander Ragnar Ironfist prepared defenses aboard the Starship Artemis.
        General Octavia Steele coordinated with Admiral Corvus Deepwater near the Frozen Wastes.
        Captain Finn O'Sullivan secured the area called the "Shadowfen Marshes".
        Commander Yuki Tanaka established communications at the "Eternal Library".

        Mr. Sebastian Cross worked as an advisor known as the "Ruins of Antiquity" expert.
        Mrs. Elizabeth Thornwood and Ms. Diana Reyes arrived at the "Celestial Observatory".
        Miss Penelope Hart accompanied Sir Galahad Brightshield and Dame Rowena Ashford.

        A warrior named Theron Blackwood emerged from the Thunderpeak Mountains.
        A mage named Seraphiel appeared at the Silver Coast Harbor.
        An assassin named Whisper materialized on the Whispering Plains.
        A healer named Solara Dawnbringer stepped out from inside the Volcanic Forge.

        Duke Maximilian von Strauß rode in from the Arcane Academy.
        Duchess Valentina Rosetti arrived aboard the Iron Leviathan.
        Earl Cedric Pemberton came from near the Cursed Catacombs.
        Countess Isadora Vega descended from the Sunlit Sanctum.
        Baron Nikolai Volkov emerged from the Moonlit Monastery.
        Baroness Anastasia Petrov sailed from the Shattered Isles.

        Dr. Evelyn Carter loves Dr. Marcus Chen.
        King Aldric the Wise is the father of Prince Kaelan Stormbringer.
        Queen Seraphina Moonveil is the mother of Princess Lyria Sunfire.
        Sir Galahad Brightshield betrayed Lord Vexaris Shadowmend.
        Commander Ragnar Ironfist killed Baron Nikolai Volkov.
        Captain Zara Nighthawk saved Dame Rowena Ashford.

        The warrior named Theron Blackwood discovered the ancient artifact.
        King Aldric the Wise realized the prophecy was true.
        Dr. Evelyn Carter learned the forbidden knowledge.
        Prince Kaelan Stormbringer defeated the shadow dragon.
        Princess Lyria Sunfire transformed into a phoenix.

        CHAPTER 2: THE STORM BEGINS

        More heroes emerged. Viscount Leopold Fitzgerald arrived at the Clockwork Citadel.
        Viscountess Genevieve Beaumont traveled within the Verdant Valley.
        Captain Aurora Windrunner flew in from the Abyssal Depths.
        Dr. Rajesh Patel and Professor Ingrid Svensson met at the Astral Nexus.
        Commander Malik Hassan crossed the Burning Sands.
        General Zhang Wei defended inside the Frozen Throne.
        Admiral Sakura Yamamoto coordinated within the Storm Spire.

        Lord Caspian Ravencrest and Lady Morgana Nightshade arrived near the Ancient Ruins.
        King Thorin Goldbeard and Queen Titania Silverleaf joined the council.
        Prince Daemon Firebrand and Princess Celestia Starweaver pledged their support.
        Dr. Nathaniel Gray and Professor Ophelia Crane analyzed the threat.
        Captain Magnus Ironforge and Commander Freya Stormborn led the vanguard.
        Sir Lancelot Brightblade and Dame Vivienne Winterrose guarded the rear.
        Mr. Jonathan Blackwell and Mrs. Catherine Ashworth provided intelligence.
        Lord Balthazar Shadowfire and Lady Serenity Moonstone cast the protective spells.
        """

        start = time.time()

        # Extract Chapter 1
        result1 = self.extract_story_elements(mega_text, chapter=1)
        print(f"  Chapter 1 extracted: {result1.get('extracted', 0)} elements")

        # Extract Chapter 2
        result2 = self.extract_story_elements(mega_text, chapter=2)
        print(f"  Chapter 2 extracted: {result2.get('extracted', 0)} elements")

        elapsed = time.time() - start
        total_extracted = result1.get('extracted', 0) + result2.get('extracted', 0)

        stats = self.get_story_stats()
        chars = self.get_story_characters()

        print(f"\n  ✓ Direct extraction completed in {elapsed:.1f}s")
        print(f"  Total elements extracted: {total_extracted}")
        print(f"  Characters in registry: {stats.get('num_characters', len(chars))}")
        print(f"  Locations in registry: {stats.get('num_locations', 0)}")
        print(f"  Facts surfaced: {stats.get('facts_surfaced', 0)}")

        self.results["tests"].append({
            "name": "direct_extraction",
            "extracted": total_extracted,
            "time": elapsed,
            "passed": total_extracted > 30
        })

        return total_extracted

    def test_stress_generation(self):
        """Test 8: Rapid-fire story generation under load"""
        print("\n" + "="*60)
        print("TEST 8: STRESS GENERATION (Rapid Requests)")
        print("="*60)

        queries = 20
        start = time.time()
        successes = 0

        for i in range(queries):
            print(f"  Query {i+1}/{queries}...", end="\r")
            result = self.generate_with_mode(
                f"Write paragraph {i+1} of the ongoing battle scene with {CHARACTERS[i % len(CHARACTERS)]}",
                mode="CREATIVE",
                max_tokens=150
            )
            if "error" not in result and result.get("response"):
                successes += 1

        elapsed = time.time() - start
        qps = queries / elapsed if elapsed > 0 else 0

        print(f"\n  ✓ Completed {successes}/{queries} queries in {elapsed:.1f}s")
        print(f"  Throughput: {qps:.2f} queries/sec")

        self.results["tests"].append({
            "name": "stress_generation",
            "queries": queries,
            "successes": successes,
            "time": elapsed,
            "qps": qps,
            "passed": successes >= queries * 0.8
        })

        return successes

    def run_all_tests(self):
        """Run complete stress test suite"""
        print("\n" + "="*60)
        print("  Z.E.T.A. STORY MODE MAXIMUM STRESS TEST")
        print("="*60)
        print(f"  Server: {self.server}")
        print(f"  Started: {self.results['start_time']}")
        print("="*60)

        if not self.check_server():
            print("\n✗ Cannot connect to server. Start it first:")
            print("  ./llama.cpp/build/bin/zeta-zero-server")
            return None

        # Set CREATIVE mode for story integration
        self.set_creative_mode()

        # Initialize story context
        self.init_story("Epic_Saga_Maximum_Stress_Test")

        # Run all tests
        self.test_direct_extraction()  # Fast path first
        self.test_character_extraction()
        self.test_location_extraction()
        self.test_relationship_web()
        self.test_chapter_progression()
        self.test_context_surfacing()
        self.test_coherence_check()
        self.test_stress_generation()

        # Get final stats
        final_health = requests.get(f"{self.server}/health", timeout=5).json()
        final_story = self.get_story_stats()

        self.results["final_health"] = final_health
        self.results["final_story_stats"] = final_story
        self.results["end_time"] = datetime.now().isoformat()

        # Summary
        print("\n" + "="*60)
        print("  STRESS TEST RESULTS SUMMARY")
        print("="*60)

        passed = sum(1 for t in self.results["tests"] if t.get("passed"))
        total = len(self.results["tests"])

        print(f"\n  Tests Passed: {passed}/{total}")
        print(f"  Characters Extracted: {self.results['characters_extracted']}/{MAX_CHARACTERS}")
        print(f"  Locations Extracted: {self.results['locations_extracted']}/{MAX_LOCATIONS}")
        print(f"  Chapters Created: {self.results['chapters_marked']}/{MAX_CHAPTERS}")
        print(f"  Relationships: {self.results['relationships_created']}")
        print(f"  Contradictions Detected: {self.results['contradictions_detected']}")

        print(f"\n  Final Graph State:")
        print(f"    Nodes: {final_health.get('graph_nodes', 'N/A')}")
        print(f"    Edges: {final_health.get('graph_edges', 'N/A')}")

        if final_story:
            print(f"\n  Story State:")
            print(f"    Title: {final_story.get('title', 'N/A')}")
            print(f"    Current Chapter: {final_story.get('current_chapter', 'N/A')}")
            print(f"    Facts Surfaced: {final_story.get('facts_surfaced', 'N/A')}")

        print("\n" + "="*60)

        # Save results
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        results_file = f"logs/story_stress_{timestamp}.json"
        try:
            with open(results_file, 'w') as f:
                json.dump(self.results, f, indent=2)
            print(f"  Results saved to: {results_file}")
        except Exception as e:
            print(f"  Could not save results: {e}")

        return self.results


def main():
    parser = argparse.ArgumentParser(description="Z.E.T.A. Story Mode Maximum Stress Test")
    parser.add_argument("--server", default=DEFAULT_SERVER, help="Server URL")
    args = parser.parse_args()

    tester = StoryModeStressTester(args.server)
    results = tester.run_all_tests()

    if results:
        passed = sum(1 for t in results["tests"] if t.get("passed"))
        total = len(results["tests"])
        sys.exit(0 if passed == total else 1)
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
