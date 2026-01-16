#!/usr/bin/env python3
"""
Fetch papers from arxiv and ingest into a Zeta research project.

Usage:
    python arxiv_ingest.py PROJECT_ID ARXIV_ID [ARXIV_ID ...]
    python arxiv_ingest.py PROJECT_ID --search "transformer attention"

Examples:
    python arxiv_ingest.py proj_abc123 1706.03762  # Attention Is All You Need
    python arxiv_ingest.py proj_abc123 1810.04805 1901.02860  # BERT, GPT-2
    python arxiv_ingest.py proj_abc123 --search "flash attention" --max 5
"""

import sys
import json
import urllib.request
import urllib.parse
import xml.etree.ElementTree as ET
import argparse
import time
import tempfile
import os

ZETA_SERVER = "http://localhost:8080"
ARXIV_API = "http://export.arxiv.org/api/query"

# Try to import pypdf for PDF extraction
try:
    from pypdf import PdfReader
    HAS_PYPDF = True
except ImportError:
    HAS_PYPDF = False

def fetch_pdf_text(arxiv_id: str, max_chars: int = 15000, save_dir: str = None):
    """Download and extract text from arxiv PDF.

    Returns dict with:
        - text: extracted text
        - pdf_path: path to saved PDF (if save_dir provided)
        - pages: list of (page_num, char_start, char_end) for each page
    """
    result = {'text': '', 'pdf_path': '', 'pages': []}
    if not HAS_PYPDF:
        return result

    # Normalize ID
    arxiv_id = arxiv_id.replace("arxiv:", "").split("v")[0]
    pdf_url = f"https://arxiv.org/pdf/{arxiv_id}.pdf"

    try:
        # Determine save path
        if save_dir:
            os.makedirs(save_dir, exist_ok=True)
            pdf_path = os.path.join(save_dir, f"{arxiv_id}.pdf")
        else:
            pdf_path = tempfile.NamedTemporaryFile(suffix='.pdf', delete=False).name

        # Download PDF
        req = urllib.request.Request(pdf_url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=60) as response:
            with open(pdf_path, 'wb') as f:
                f.write(response.read())

        # Extract text with page boundaries
        reader = PdfReader(pdf_path)
        text = ""
        pages = []
        for page_num, page in enumerate(reader.pages):
            char_start = len(text)
            page_text = page.extract_text()
            if page_text:
                text += f"[PAGE {page_num + 1}]\n{page_text}\n"
            char_end = len(text)
            pages.append((page_num + 1, char_start, char_end))
            if len(text) > max_chars:
                break

        # Clean up temp file if not saving
        if not save_dir:
            os.unlink(pdf_path)
            pdf_path = ""

        result = {
            'text': text[:max_chars],
            'pdf_path': pdf_path,
            'pages': pages
        }
        return result

    except Exception as e:
        print(f"    PDF extraction failed: {e}")
        return result

def fetch_arxiv_by_id(arxiv_id: str) -> dict:
    """Fetch paper metadata from arxiv by ID."""
    # Normalize ID (remove version, arxiv: prefix)
    arxiv_id = arxiv_id.replace("arxiv:", "").split("v")[0]

    url = f"{ARXIV_API}?id_list={arxiv_id}"
    try:
        with urllib.request.urlopen(url, timeout=30) as response:
            xml_data = response.read().decode('utf-8')
    except Exception as e:
        print(f"Error fetching {arxiv_id}: {e}")
        return None

    return parse_arxiv_response(xml_data)

def search_arxiv(query: str, max_results: int = 10) -> list:
    """Search arxiv for papers matching query."""
    params = urllib.parse.urlencode({
        'search_query': f'all:{query}',
        'start': 0,
        'max_results': max_results,
        'sortBy': 'relevance',
        'sortOrder': 'descending'
    })

    url = f"{ARXIV_API}?{params}"
    try:
        with urllib.request.urlopen(url, timeout=30) as response:
            xml_data = response.read().decode('utf-8')
    except Exception as e:
        print(f"Error searching arxiv: {e}")
        return []

    return parse_arxiv_response(xml_data, multiple=True)

def parse_arxiv_response(xml_data: str, multiple: bool = False):
    """Parse arxiv API XML response."""
    ns = {'atom': 'http://www.w3.org/2005/Atom'}

    root = ET.fromstring(xml_data)
    entries = root.findall('atom:entry', ns)

    results = []
    for entry in entries:
        title = entry.find('atom:title', ns)
        summary = entry.find('atom:summary', ns)
        published = entry.find('atom:published', ns)

        authors = []
        for author in entry.findall('atom:author', ns):
            name = author.find('atom:name', ns)
            if name is not None:
                authors.append(name.text.strip())

        # Get arxiv ID from the id URL
        id_elem = entry.find('atom:id', ns)
        arxiv_id = ""
        if id_elem is not None:
            arxiv_id = id_elem.text.split('/')[-1]

        # Get categories
        categories = []
        for cat in entry.findall('atom:category', ns):
            term = cat.get('term')
            if term:
                categories.append(term)

        paper = {
            'arxiv_id': arxiv_id,
            'title': title.text.strip().replace('\n', ' ') if title is not None else "Unknown",
            'authors': authors,
            'abstract': summary.text.strip().replace('\n', ' ') if summary is not None else "",
            'published': published.text if published is not None else "",
            'categories': categories
        }
        results.append(paper)

    if multiple:
        return results
    return results[0] if results else None

def ingest_paper(project_id: str, paper: dict, server: str = ZETA_SERVER, use_pdf: bool = False, pdf_dir: str = None) -> bool:
    """Ingest a paper into a Zeta research project."""
    # Try to get PDF text if requested
    pdf_result = {'text': '', 'pdf_path': '', 'pages': []}
    if use_pdf and HAS_PYPDF:
        print(f"    Fetching PDF...")
        pdf_result = fetch_pdf_text(paper['arxiv_id'], save_dir=pdf_dir)
        if pdf_result['text']:
            print(f"    Extracted {len(pdf_result['text'])} chars from PDF")
            if pdf_result['pdf_path']:
                print(f"    Saved to: {pdf_result['pdf_path']}")

    # Build content - use PDF text if available, otherwise abstract
    pdf_text = pdf_result['text']
    pdf_path = pdf_result['pdf_path']

    if pdf_text:
        content = f"""Title: {paper['title']}

Authors: {', '.join(paper['authors'])}

Published: {paper['published'][:10] if paper['published'] else 'Unknown'}

PDF: {pdf_path if pdf_path else 'not saved'}

Full Paper Text:
{pdf_text}
"""
    else:
        content = f"""Title: {paper['title']}

Authors: {', '.join(paper['authors'])}

Published: {paper['published'][:10] if paper['published'] else 'Unknown'}

Categories: {', '.join(paper['categories'][:5])}

Abstract:
{paper['abstract']}
"""

    payload = {
        'title': paper['title'],
        'content': content,
        'source': f"arxiv:{paper['arxiv_id']}",
        'source_type': 'arxiv'
    }

    url = f"{server}/research/project/{project_id}/ingest"
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode('utf-8'),
        headers={'Content-Type': 'application/json'},
        method='POST'
    )

    try:
        with urllib.request.urlopen(req, timeout=120) as response:
            result = json.loads(response.read().decode('utf-8'))
            print(f"  Ingested: {paper['title'][:60]}...")
            print(f"    Nodes: {result.get('graph_nodes', 0)}, Edges: {result.get('graph_edges', 0)}")
            return True
    except urllib.error.HTTPError as e:
        print(f"  Error ingesting {paper['arxiv_id']}: HTTP {e.code}")
        return False
    except Exception as e:
        print(f"  Error ingesting {paper['arxiv_id']}: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='Ingest arxiv papers into Zeta research project')
    parser.add_argument('project_id', help='Zeta project ID (e.g., proj_abc123)')
    parser.add_argument('arxiv_ids', nargs='*', help='Arxiv paper IDs (e.g., 1706.03762)')
    parser.add_argument('--search', '-s', help='Search query to find papers')
    parser.add_argument('--max', '-m', type=int, default=10, help='Max papers for search (default: 10)')
    parser.add_argument('--server', default=ZETA_SERVER, help=f'Zeta server URL (default: {ZETA_SERVER})')
    parser.add_argument('--delay', type=float, default=0.5, help='Delay between requests (default: 0.5s)')
    parser.add_argument('--pdf', action='store_true', help='Extract full text from PDFs (requires pypdf)')
    parser.add_argument('--pdf-dir', help='Directory to save PDFs for reference (enables deep linking)')

    args = parser.parse_args()

    if not args.arxiv_ids and not args.search:
        parser.error("Provide arxiv IDs or --search query")

    papers = []

    # Fetch by IDs
    if args.arxiv_ids:
        print(f"Fetching {len(args.arxiv_ids)} papers from arxiv...")
        for arxiv_id in args.arxiv_ids:
            paper = fetch_arxiv_by_id(arxiv_id)
            if paper:
                papers.append(paper)
                print(f"  Found: {paper['title'][:60]}...")
            time.sleep(args.delay)  # Be nice to arxiv API

    # Search
    if args.search:
        print(f"Searching arxiv for: {args.search}")
        search_results = search_arxiv(args.search, args.max)
        papers.extend(search_results)
        print(f"  Found {len(search_results)} papers")

    if not papers:
        print("No papers found")
        return 1

    # Ingest
    if args.pdf and not HAS_PYPDF:
        print("Warning: --pdf requested but pypdf not installed. Install with: pip install pypdf")
        print("Falling back to abstracts only.\n")

    print(f"\nIngesting {len(papers)} papers into {args.project_id}...")
    success = 0
    for paper in papers:
        if ingest_paper(args.project_id, paper, args.server, use_pdf=args.pdf, pdf_dir=args.pdf_dir):
            success += 1
        time.sleep(args.delay)

    print(f"\nDone: {success}/{len(papers)} papers ingested")
    return 0 if success == len(papers) else 1

if __name__ == '__main__':
    sys.exit(main())
