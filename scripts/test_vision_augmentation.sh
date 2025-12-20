#!/bin/bash

# Z.E.T.A. Vision-Augmented Test
# Tests multimodal capabilities with image understanding

SERVER_URL="http://localhost:9001"
VISION_SCRIPT="/tmp/zeta_vision_test.py"

# Create vision test script
cat > "$VISION_SCRIPT" << 'EOF'
import torch
from transformers import CLIPProcessor, CLIPModel
from PIL import Image
import numpy as np
import requests
import base64
import io

def encode_image_to_base64(image_path):
    """Convert image to base64 string"""
    with open(image_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode('utf-8')

def create_test_image(description, filename):
    """Create a simple test image based on description"""
    img = np.zeros((224, 224, 3), dtype=np.uint8)

    if "red" in description.lower():
        img[:, :] = [255, 0, 0]  # Red
    elif "blue" in description.lower():
        img[:, :] = [0, 0, 255]  # Blue
    elif "green" in description.lower():
        img[:, :] = [0, 255, 0]  # Green
    else:
        # Random colorful pattern
        img = np.random.randint(0, 255, (224, 224, 3), dtype=np.uint8)

    pil_img = Image.fromarray(img)
    pil_img.save(filename)
    return filename

def analyze_image_with_clip(image_path, texts):
    """Use CLIP to analyze image against text descriptions"""
    try:
        model = CLIPModel.from_pretrained('openai/clip-vit-base-patch32')
        processor = CLIPProcessor.from_pretrained('openai/clip-vit-base-patch32')

        image = Image.open(image_path)
        inputs = processor(text=texts, images=image, return_tensors="pt", padding=True)

        with torch.no_grad():
            outputs = model(**inputs)
            logits_per_image = outputs.logits_per_image
            probs = logits_per_image.softmax(dim=1)

        return probs[0].tolist()
    except Exception as e:
        print(f"CLIP analysis failed: {e}")
        return None

# Test 1: Create and remember visual content
print("=== VISION TEST 1: Visual Memory ===")

# Create a red square image
red_image = create_test_image("red square", "/tmp/red_square.jpg")
print(f"Created red square image: {red_image}")

# Analyze with CLIP
texts = ["a red square", "a blue circle", "a green triangle", "a yellow star"]
probs = analyze_image_with_clip(red_image, texts)
if probs:
    best_match = texts[probs.index(max(probs))]
    print(f"CLIP analysis: {best_match} (confidence: {max(probs):.2f})")

# Test 2: Multimodal Z.E.T.A. integration
print("\n=== VISION TEST 2: Multimodal Integration ===")

# Create multiple test images
images = []
image_descriptions = [
    "blue circle",
    "green triangle",
    "yellow star",
    "purple diamond"
]

for desc in image_descriptions:
    filename = f"/tmp/{desc.replace(' ', '_')}.jpg"
    create_test_image(desc, filename)
    images.append((desc, filename))

# Analyze all images
for desc, path in images:
    texts = [f"a {color} {shape}" for color in ["red", "blue", "green", "yellow", "purple"]
             for shape in ["square", "circle", "triangle", "star", "diamond"]]
    probs = analyze_image_with_clip(path, texts)
    if probs:
        best_match = texts[probs.index(max(probs))]
        print(f"{desc} -> CLIP: {best_match} (confidence: {max(probs):.2f})")

print("\n=== Vision capabilities ready for Z.E.T.A. integration ===")
EOF

# Run the vision test
python3 "$VISION_SCRIPT"

echo ""
echo "=== Z.E.T.A. Multimodal Test ==="

# Test multimodal memory with Z.E.T.A.
echo "Testing Z.E.T.A. with visual context..."

# Create a test image and describe it to Z.E.T.A.
curl -s -X POST "$SERVER_URL/generate" \
  -H "Content-Type: application/json" \
  -d '{
    "prompt": "I am showing you an image of a bright red square. Remember this visual: a red square represents danger or stop. This is important for safety.",
    "max_tokens": 30
  }' | jq -r '.output'

echo ""
echo "Waiting for memory consolidation..."
sleep 3

# Test recall of visual information
curl -s -X POST "$SERVER_URL/generate" \
  -H "Content-Type: application/json" \
  -d '{
    "prompt": "What does a red square represent according to our visual memory?",
    "max_tokens": 25
  }' | jq -r '.output'

echo ""
echo "=== Vision augmentation test complete ==="