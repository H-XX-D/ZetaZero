#!/bin/bash
# Z.E.T.A. Dream Demo Recorder
# Records a dream cycle for promotional content

set -e

DEMO_DIR="${DEMO_DIR:-$HOME/.zetazero/demos}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_FILE="$DEMO_DIR/dream_demo_$TIMESTAMP"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}=========================================="
echo "  Z.E.T.A. Dream Demo Recorder"
echo -e "==========================================${NC}"

# Create demo directory
mkdir -p "$DEMO_DIR"

# Check for asciinema
if ! command -v asciinema &> /dev/null; then
    echo -e "${YELLOW}Installing asciinema...${NC}"
    if command -v apt-get &> /dev/null; then
        sudo apt-get install -y asciinema
    elif command -v brew &> /dev/null; then
        brew install asciinema
    else
        pip3 install asciinema
    fi
fi

# Check for svg-term (for GIF-like output)
if ! command -v svg-term &> /dev/null; then
    echo -e "${YELLOW}For best results, install svg-term: npm install -g svg-term-cli${NC}"
fi

echo ""
echo -e "${GREEN}Recording will capture:${NC}"
echo "  1. Server startup"
echo "  2. Dream state entry"
echo "  3. Dream generation"
echo ""
echo "Press Ctrl+C when you have a good dream captured."
echo ""
read -p "Press Enter to start recording..."

# Record the session
echo -e "${CYAN}Recording started...${NC}"
asciinema rec "$OUTPUT_FILE.cast" \
    --title "Z.E.T.A. Dream State Demo" \
    --idle-time-limit 2 \
    -c "docker logs -f zetazero 2>&1 | head -200"

echo ""
echo -e "${GREEN}Recording saved to: $OUTPUT_FILE.cast${NC}"

# Convert to SVG (animated, works in browsers)
if command -v svg-term &> /dev/null; then
    echo -e "${YELLOW}Converting to SVG...${NC}"
    svg-term --in "$OUTPUT_FILE.cast" --out "$OUTPUT_FILE.svg" --window
    echo -e "${GREEN}SVG saved to: $OUTPUT_FILE.svg${NC}"
fi

# Convert to GIF if possible
if command -v agg &> /dev/null; then
    echo -e "${YELLOW}Converting to GIF...${NC}"
    agg "$OUTPUT_FILE.cast" "$OUTPUT_FILE.gif"
    echo -e "${GREEN}GIF saved to: $OUTPUT_FILE.gif${NC}"
elif command -v gifski &> /dev/null; then
    echo -e "${YELLOW}Use: asciinema-agg or gifski to convert to GIF${NC}"
fi

echo ""
echo -e "${CYAN}=========================================="
echo "  Demo files ready!"
echo "==========================================${NC}"
echo ""
echo "Files created:"
echo "  - $OUTPUT_FILE.cast (asciinema format)"
[ -f "$OUTPUT_FILE.svg" ] && echo "  - $OUTPUT_FILE.svg (animated SVG)"
[ -f "$OUTPUT_FILE.gif" ] && echo "  - $OUTPUT_FILE.gif (GIF)"
echo ""
echo "To replay: asciinema play $OUTPUT_FILE.cast"
echo "To upload: asciinema upload $OUTPUT_FILE.cast"
echo ""
echo "For Twitter/Reddit, use the GIF or upload to asciinema.org"
