#!/usr/bin/env python3
"""
Z.E.T.A. Codebase Indexer
Parses source code and populates the memory graph with self-knowledge nodes.
This enables graph jumps to land on actual codebase elements during dreaming.
"""

import os
import re
import json
import requests
import argparse
from pathlib import Path

ZETA_URL = "http://192.168.0.165:8080"

# Patterns for C++ extraction
PATTERNS = {
    'function': re.compile(
        r'(?:static\s+)?(?:inline\s+)?'
        r'(?P<return_type>[\w\s\*&:<>]+?)\s+'
        r'(?P<name>\w+)\s*\('
        r'(?P<params>[^)]*)\)\s*(?:const\s*)?(?:override\s*)?{'
    ),
    'struct': re.compile(
        r'(?:typedef\s+)?struct\s+(?P<name>\w+)\s*{'
    ),
    'class': re.compile(
        r'class\s+(?P<name>\w+)\s*(?::\s*(?:public|private|protected)\s+\w+)?\s*{'
    ),
    'define': re.compile(
        r'#define\s+(?P<name>\w+)\s+(?P<value>.+?)(?:\n|$)'
    ),
    'comment_block': re.compile(
        r'/\*\*?\s*(?P<content>[\s\S]*?)\*/',
        re.MULTILINE
    ),
    'comment_section': re.compile(
        r'//\s*={10,}\s*\n//\s*(?P<title>[^\n]+)\n//\s*={10,}'
    ),
}


def extract_functions(content: str, filename: str) -> list:
    """Extract function definitions from C++ code."""
    nodes = []

    for match in PATTERNS['function'].finditer(content):
        name = match.group('name')
        return_type = match.group('return_type').strip()
        params = match.group('params').strip()

        # Skip common false positives
        if name in ['if', 'while', 'for', 'switch', 'catch']:
            continue

        # Get surrounding context (line number, nearby comments)
        start = match.start()
        line_num = content[:start].count('\n') + 1

        # Look for comment above function
        comment = ""
        lines_before = content[:start].split('\n')[-5:]
        for line in lines_before:
            if '//' in line:
                comment += line.split('//')[-1].strip() + " "

        nodes.append({
            'label': f'function:{name}',
            'value': f'{return_type} {name}({params[:100]}...) - {comment[:200]}',
            'type': 'function',
            'file': filename,
            'line': line_num,
            'salience': 0.9
        })

    return nodes


def extract_structs(content: str, filename: str) -> list:
    """Extract struct/class definitions."""
    nodes = []

    for pattern_name in ['struct', 'class']:
        for match in PATTERNS[pattern_name].finditer(content):
            name = match.group('name')
            start = match.start()
            line_num = content[:start].count('\n') + 1

            # Get struct body preview (first 200 chars)
            body_start = match.end()
            body_preview = content[body_start:body_start+300].split('}')[0]

            nodes.append({
                'label': f'{pattern_name}:{name}',
                'value': f'{pattern_name} {name} {{ {body_preview[:150]}... }}',
                'type': pattern_name,
                'file': filename,
                'line': line_num,
                'salience': 0.95
            })

    return nodes


def extract_defines(content: str, filename: str) -> list:
    """Extract important #defines."""
    nodes = []

    for match in PATTERNS['define'].finditer(content):
        name = match.group('name')
        value = match.group('value').strip()

        # Skip include guards and common macros
        if name.endswith('_H') or name.startswith('_'):
            continue

        nodes.append({
            'label': f'define:{name}',
            'value': f'#define {name} {value[:100]}',
            'type': 'define',
            'file': filename,
            'salience': 0.7
        })

    return nodes


def extract_sections(content: str, filename: str) -> list:
    """Extract section headers (// ======= SECTION =======)."""
    nodes = []

    for match in PATTERNS['comment_section'].finditer(content):
        title = match.group('title').strip()
        start = match.start()
        line_num = content[:start].count('\n') + 1

        nodes.append({
            'label': f'section:{title}',
            'value': f'Code section: {title} in {filename}',
            'type': 'section',
            'file': filename,
            'line': line_num,
            'salience': 0.8
        })

    return nodes


def index_file(filepath: str) -> list:
    """Index a single source file."""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception as e:
        print(f"  Error reading {filepath}: {e}")
        return []

    filename = os.path.basename(filepath)
    nodes = []

    nodes.extend(extract_functions(content, filename))
    nodes.extend(extract_structs(content, filename))
    nodes.extend(extract_defines(content, filename))
    nodes.extend(extract_sections(content, filename))

    return nodes


def commit_node(node: dict, server_url: str) -> bool:
    """Commit a node to the Z.E.T.A. graph."""
    try:
        response = requests.post(
            f"{server_url}/git/commit",
            json={
                'label': node['label'],
                'value': node['value'],
                'salience': node.get('salience', 0.85)
            },
            timeout=10
        )
        return response.status_code == 200
    except Exception as e:
        print(f"  Error committing {node['label']}: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(description='Index Z.E.T.A. codebase into memory graph')
    parser.add_argument('--path', default='/home/hendrixx/ZetaZero/llama.cpp/tools/zeta-demo',
                        help='Path to source code')
    parser.add_argument('--server', default=ZETA_URL, help='Z.E.T.A. server URL')
    parser.add_argument('--dry-run', action='store_true', help='Parse but do not commit')
    parser.add_argument('--branch', default='self', help='Branch name for codebase nodes')
    args = parser.parse_args()

    print(f"Z.E.T.A. Codebase Indexer")
    print(f"========================")
    print(f"Source path: {args.path}")
    print(f"Server: {args.server}")
    print()

    # Create self branch for codebase knowledge
    if not args.dry_run:
        try:
            requests.post(f"{args.server}/git/branch", json={'name': args.branch}, timeout=5)
            requests.post(f"{args.server}/git/checkout", json={'name': args.branch}, timeout=5)
            print(f"Switched to branch: {args.branch}")
        except Exception as e:
            print(f"Warning: Could not create/switch branch: {e}")

    # Find all source files
    source_files = []
    for ext in ['*.cpp', '*.h', '*.hpp']:
        source_files.extend(Path(args.path).glob(ext))

    print(f"Found {len(source_files)} source files")
    print()

    total_nodes = 0
    committed = 0

    for filepath in sorted(source_files):
        print(f"Indexing: {filepath.name}")
        nodes = index_file(str(filepath))

        for node in nodes:
            total_nodes += 1
            if not args.dry_run:
                if commit_node(node, args.server):
                    committed += 1
                    print(f"  + {node['label']}")
            else:
                print(f"  [dry-run] {node['label']}: {node['value'][:60]}...")

    print()
    print(f"Summary:")
    print(f"  Total nodes extracted: {total_nodes}")
    if not args.dry_run:
        print(f"  Nodes committed: {committed}")

    # Switch back to main
    if not args.dry_run:
        try:
            requests.post(f"{args.server}/git/checkout", json={'name': 'main'}, timeout=5)
            print(f"Switched back to main branch")
        except:
            pass


if __name__ == '__main__':
    main()
