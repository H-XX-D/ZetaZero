#!/usr/bin/env python3
"""
Z.E.T.A. Dream Review System
Human-in-the-loop validation with adaptive thresholds

Usage: python3 dream_review.py [--remote xx@192.168.0.165]
"""

import os
import sys
import json
import time
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Optional, Dict, List

# ============================================================================
# Configuration
# ============================================================================

REMOTE_HOST = "xx@192.168.0.165"
REMOTE_DREAMS_PATH = "/mnt/GitGraph/dreams/pending"
REMOTE_APPROVED_PATH = "/mnt/GitGraph/dreams/approved"
REMOTE_REJECTED_PATH = "/mnt/GitGraph/dreams/rejected"
LOCAL_STATS_PATH = os.path.expanduser("~/.zeta/dream_review_stats.json")

TARGET_ACCEPTANCE_RATE = 0.70  # Adaptive threshold targets 70% acceptance
MIN_NOVELTY_THRESHOLD = 0.20
MAX_NOVELTY_THRESHOLD = 0.90

# Category colors (ANSI)
COLORS = {
    'code_idea': '\033[96m',    # Cyan
    'code_fix': '\033[92m',     # Green
    'insight': '\033[93m',      # Yellow
    'story': '\033[95m',        # Magenta
    'pattern': '\033[94m',      # Blue
    'connection': '\033[91m',   # Red
    'memory': '\033[97m',       # White
    'reset': '\033[0m',
    'bold': '\033[1m',
    'dim': '\033[2m',
}

# ============================================================================
# Stats Persistence
# ============================================================================

def load_stats() -> Dict:
    """Load review stats from disk"""
    os.makedirs(os.path.dirname(LOCAL_STATS_PATH), exist_ok=True)
    if os.path.exists(LOCAL_STATS_PATH):
        with open(LOCAL_STATS_PATH, 'r') as f:
            return json.load(f)
    return {
        'categories': {},
        'total_reviewed': 0,
        'total_accepted': 0,
        'total_rejected': 0,
        'total_edited': 0,
        'sessions': [],
        'novelty_threshold': 0.30,  # Current adaptive threshold
    }

def save_stats(stats: Dict):
    """Save review stats to disk"""
    with open(LOCAL_STATS_PATH, 'w') as f:
        json.dump(stats, f, indent=2)

def update_category_stats(stats: Dict, category: str, accepted: bool, edited: bool = False):
    """Update per-category acceptance rates"""
    if category not in stats['categories']:
        stats['categories'][category] = {
            'total': 0,
            'accepted': 0,
            'rejected': 0,
            'edited': 0,
            'acceptance_rate': 0.0
        }
    
    cat = stats['categories'][category]
    cat['total'] += 1
    if accepted:
        cat['accepted'] += 1
        if edited:
            cat['edited'] += 1
    else:
        cat['rejected'] += 1
    
    cat['acceptance_rate'] = cat['accepted'] / cat['total'] if cat['total'] > 0 else 0.0

def adapt_threshold(stats: Dict):
    """Adjust novelty threshold based on acceptance rate"""
    if stats['total_reviewed'] < 10:
        return  # Need minimum samples
    
    overall_rate = stats['total_accepted'] / stats['total_reviewed']
    current = stats['novelty_threshold']
    
    # If accepting too many, raise threshold (be more selective)
    # If rejecting too many, lower threshold (be less selective)
    if overall_rate > TARGET_ACCEPTANCE_RATE + 0.1:
        stats['novelty_threshold'] = min(current + 0.05, MAX_NOVELTY_THRESHOLD)
    elif overall_rate < TARGET_ACCEPTANCE_RATE - 0.1:
        stats['novelty_threshold'] = max(current - 0.05, MIN_NOVELTY_THRESHOLD)

# ============================================================================
# Remote Operations
# ============================================================================

def ssh_cmd(cmd: str) -> str:
    """Execute SSH command and return output"""
    result = subprocess.run(
        ['ssh', REMOTE_HOST, cmd],
        capture_output=True, text=True, timeout=30
    )
    return result.stdout.strip()

def scp_get(remote_path: str, local_path: str):
    """Copy file from remote"""
    subprocess.run(['scp', f'{REMOTE_HOST}:{remote_path}', local_path], 
                   capture_output=True, timeout=30)

def get_pending_dreams() -> List[Dict]:
    """Get list of pending dreams from remote"""
    dreams = []
    
    # List files
    files = ssh_cmd(f'ls -t {REMOTE_DREAMS_PATH}/*.txt 2>/dev/null || echo ""')
    if not files:
        return dreams
    
    for filepath in files.split('\n')[:20]:  # Limit to 20 most recent
        if not filepath.strip():
            continue
        
        # Get file content
        content = ssh_cmd(f'cat "{filepath}"')
        if not content:
            continue
        
        lines = content.split('\n')
        if len(lines) < 3:
            continue
        
        category = lines[0].strip()
        timestamp = lines[1].strip()
        body = '\n'.join(lines[2:]).strip()
        
        # Extract filename for later operations
        filename = os.path.basename(filepath)
        
        dreams.append({
            'filepath': filepath,
            'filename': filename,
            'category': category,
            'timestamp': timestamp,
            'body': body,
            'preview': body[:200].replace('\n', ' ')
        })
    
    return dreams

def move_dream(filepath: str, destination: str):
    """Move dream to approved/rejected folder"""
    ssh_cmd(f'mkdir -p {destination} && mv "{filepath}" {destination}/')

def save_edited_dream(filepath: str, edited_content: str, category: str):
    """Save edited dream content"""
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    # Create a temp file and scp it
    import tempfile
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write(f"{category}\n{timestamp}\n{edited_content}")
        temp_path = f.name
    
    new_filename = f"dream_{timestamp}_{category}_edited.txt"
    subprocess.run(['scp', temp_path, f'{REMOTE_HOST}:{REMOTE_APPROVED_PATH}/{new_filename}'],
                   capture_output=True, timeout=30)
    os.unlink(temp_path)
    
    # Remove original
    ssh_cmd(f'rm "{filepath}"')

# ============================================================================
# Display Functions  
# ============================================================================

def clear_screen():
    os.system('clear' if os.name != 'nt' else 'cls')

def print_header(stats: Dict, pending_count: int):
    """Print morning briefing header"""
    c = COLORS
    now = datetime.now().strftime('%Y-%m-%d %H:%M')
    
    print(f"\n{c['bold']}╔══════════════════════════════════════════════════════════════╗{c['reset']}")
    print(f"{c['bold']}║  🌙 MORNING BRIEFING: {pending_count} insights from idle processing{c['reset']}")
    print(f"{c['bold']}╠══════════════════════════════════════════════════════════════╣{c['reset']}")
    print(f"{c['dim']}║  Date: {now}  |  Threshold: {stats['novelty_threshold']:.2f}  |  Rate: {stats['total_accepted']}/{stats['total_reviewed']}{c['reset']}")
    print(f"{c['bold']}╚══════════════════════════════════════════════════════════════╝{c['reset']}")
    print()

def print_dream(idx: int, dream: Dict):
    """Print a single dream for review"""
    c = COLORS
    cat = dream['category']
    cat_color = c.get(cat, c['reset'])
    
    print(f"{c['bold']}[{idx}] {cat_color}{cat.upper()}{c['reset']}: {dream['timestamp']}")
    print(f"    {c['dim']}─────────────────────────────────────────────────────{c['reset']}")
    
    # Show preview (first 3 lines)
    lines = dream['body'].split('\n')[:5]
    for line in lines:
        truncated = line[:70] + '...' if len(line) > 70 else line
        print(f"    {truncated}")
    
    if len(dream['body'].split('\n')) > 5:
        print(f"    {c['dim']}... ({len(dream['body'])} chars total){c['reset']}")
    
    print(f"    {c['bold']}📎 [Y]es  [N]o  [E]dit  [D]etails  [S]kip{c['reset']}")
    print()

def print_details(dream: Dict):
    """Print full dream details"""
    c = COLORS
    clear_screen()
    print(f"\n{c['bold']}═══ FULL DREAM CONTENT ═══{c['reset']}\n")
    print(f"{c['dim']}Category: {dream['category']}{c['reset']}")
    print(f"{c['dim']}Timestamp: {dream['timestamp']}{c['reset']}")
    print(f"{c['dim']}File: {dream['filename']}{c['reset']}")
    print(f"\n{'-'*60}\n")
    print(dream['body'])
    print(f"\n{'-'*60}")
    input(f"\n{c['dim']}Press Enter to continue...{c['reset']}")

def print_stats_summary(stats: Dict):
    """Print session stats summary"""
    c = COLORS
    print(f"\n{c['bold']}═══ SESSION STATS ═══{c['reset']}")
    print(f"  Total Reviewed: {stats['total_reviewed']}")
    print(f"  Accepted: {stats['total_accepted']} ({100*stats['total_accepted']/max(1,stats['total_reviewed']):.0f}%)")
    print(f"  Rejected: {stats['total_rejected']}")
    print(f"  Edited: {stats['total_edited']}")
    print(f"  Adaptive Threshold: {stats['novelty_threshold']:.2f}")
    print()
    
    if stats['categories']:
        print(f"{c['bold']}Per-Category Acceptance:{c['reset']}")
        for cat, data in sorted(stats['categories'].items()):
            rate = data['acceptance_rate'] * 100
            bar = '█' * int(rate/10) + '░' * (10 - int(rate/10))
            color = c.get(cat, c['reset'])
            print(f"  {color}{cat:12}{c['reset']} [{bar}] {rate:.0f}% ({data['accepted']}/{data['total']})")
    print()

def edit_dream(dream: Dict) -> Optional[str]:
    """Open editor for dream content"""
    import tempfile
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write(dream['body'])
        temp_path = f.name
    
    # Use $EDITOR or fall back to nano/vim
    editor = os.environ.get('EDITOR', 'nano')
    os.system(f'{editor} {temp_path}')
    
    with open(temp_path, 'r') as f:
        edited = f.read()
    
    os.unlink(temp_path)
    
    if edited.strip() != dream['body'].strip():
        return edited.strip()
    return None

# ============================================================================
# Main Review Loop
# ============================================================================

def review_session():
    """Run interactive review session"""
    stats = load_stats()
    
    print("\n🌙 Fetching pending dreams...")
    dreams = get_pending_dreams()
    
    if not dreams:
        print("✨ No pending dreams to review!")
        print_stats_summary(stats)
        return
    
    clear_screen()
    print_header(stats, len(dreams))
    
    session_accepted = 0
    session_rejected = 0
    session_edited = 0
    
    idx = 0
    while idx < len(dreams):
        dream = dreams[idx]
        print_dream(idx + 1, dream)
        
        try:
            choice = input(f"  Choice for [{idx+1}]: ").strip().lower()
        except (KeyboardInterrupt, EOFError):
            print("\n\nSession interrupted.")
            break
        
        if choice in ['y', 'yes', '1']:
            # Accept
            move_dream(dream['filepath'], REMOTE_APPROVED_PATH)
            update_category_stats(stats, dream['category'], accepted=True)
            stats['total_reviewed'] += 1
            stats['total_accepted'] += 1
            session_accepted += 1
            print(f"  ✅ Accepted → {REMOTE_APPROVED_PATH}")
            idx += 1
            
        elif choice in ['n', 'no', '2']:
            # Reject
            move_dream(dream['filepath'], REMOTE_REJECTED_PATH)
            update_category_stats(stats, dream['category'], accepted=False)
            stats['total_reviewed'] += 1
            stats['total_rejected'] += 1
            session_rejected += 1
            print(f"  ❌ Rejected → {REMOTE_REJECTED_PATH}")
            idx += 1
            
        elif choice in ['e', 'edit', '3']:
            # Edit
            edited = edit_dream(dream)
            if edited:
                save_edited_dream(dream['filepath'], edited, dream['category'])
                update_category_stats(stats, dream['category'], accepted=True, edited=True)
                stats['total_reviewed'] += 1
                stats['total_accepted'] += 1
                stats['total_edited'] += 1
                session_edited += 1
                print(f"  ✏️ Edited & Accepted")
            else:
                print(f"  {COLORS['dim']}No changes made{COLORS['reset']}")
            idx += 1
            
        elif choice in ['d', 'details', '4']:
            # Show full details
            print_details(dream)
            clear_screen()
            print_header(stats, len(dreams) - idx)
            # Don't increment idx - show same dream again
            
        elif choice in ['s', 'skip', '5']:
            # Skip for now
            print(f"  ⏭️ Skipped")
            idx += 1
            
        elif choice in ['q', 'quit']:
            break
            
        elif choice == 'stats':
            print_stats_summary(stats)
            
        else:
            print(f"  {COLORS['dim']}Unknown command. Use Y/N/E/D/S or Q to quit{COLORS['reset']}")
        
        print()
    
    # Adapt threshold based on this session
    adapt_threshold(stats)
    
    # Record session
    stats['sessions'].append({
        'timestamp': datetime.now().isoformat(),
        'reviewed': session_accepted + session_rejected,
        'accepted': session_accepted,
        'rejected': session_rejected,
        'edited': session_edited,
    })
    
    save_stats(stats)
    
    # Final summary
    clear_screen()
    print(f"\n{COLORS['bold']}═══ SESSION COMPLETE ═══{COLORS['reset']}")
    print(f"  This session: ✅ {session_accepted}  ❌ {session_rejected}  ✏️ {session_edited}")
    print_stats_summary(stats)
    
    # Show threshold adjustment
    if stats['total_reviewed'] >= 10:
        rate = stats['total_accepted'] / stats['total_reviewed']
        if rate > TARGET_ACCEPTANCE_RATE + 0.1:
            print(f"📈 Acceptance rate high ({rate:.0%}) → Threshold raised to {stats['novelty_threshold']:.2f}")
        elif rate < TARGET_ACCEPTANCE_RATE - 0.1:
            print(f"📉 Acceptance rate low ({rate:.0%}) → Threshold lowered to {stats['novelty_threshold']:.2f}")
        else:
            print(f"✓ Acceptance rate on target ({rate:.0%})")

def quick_list():
    """Quick list of pending dreams without interaction"""
    print("\n🌙 Fetching pending dreams...")
    dreams = get_pending_dreams()
    
    if not dreams:
        print("✨ No pending dreams!")
        return
    
    print(f"\n{len(dreams)} pending dreams:\n")
    for i, d in enumerate(dreams[:10]):
        cat_color = COLORS.get(d['category'], COLORS['reset'])
        print(f"  [{i+1}] {cat_color}{d['category']:12}{COLORS['reset']} {d['preview'][:50]}...")
    
    if len(dreams) > 10:
        print(f"\n  ... and {len(dreams) - 10} more")
    print(f"\nRun with no args for interactive review")

# ============================================================================
# Entry Point
# ============================================================================

if __name__ == '__main__':
    if '--list' in sys.argv or '-l' in sys.argv:
        quick_list()
    elif '--stats' in sys.argv:
        stats = load_stats()
        print_stats_summary(stats)
    elif '--help' in sys.argv or '-h' in sys.argv:
        print(__doc__)
        print("\nCommands:")
        print("  (no args)   Interactive review session")
        print("  --list, -l  Quick list of pending dreams")
        print("  --stats     Show review statistics")
        print("\nDuring review:")
        print("  Y/1  Accept dream")
        print("  N/2  Reject dream")
        print("  E/3  Edit and accept")
        print("  D/4  Show full details")
        print("  S/5  Skip for now")
        print("  Q    Quit session")
    else:
        review_session()
