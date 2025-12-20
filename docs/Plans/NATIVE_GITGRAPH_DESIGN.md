# Native GitGraph: Architecture-Native Graph Memory

## Vision

Replace the external graph layer with a model that **natively thinks in graphs**.
Instead of: `LLM output → regex extraction → graph storage`
Become: `LLM generates tokens that ARE graph operations`

---

## Current Architecture (External Graph)

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│  14B Main   │────►│  C++ Graph   │────►│  GKV Cache  │
│   Model     │     │   Engine     │     │   Storage   │
└─────────────┘     └──────────────┘     └─────────────┘
                          ▲
                          │ regex
┌─────────────┐           │
│ 3B Extract  │───────────┘
│   (regex)   │
└─────────────┘
```

**Problems:**
- Regex misses semantic meaning ("O(1) sort" → gets fooled)
- No backprop through graph operations
- Model can't learn optimal graph structure
- 3B and graph are disconnected modules

---

## Native Architecture (GitGraph)

```
┌─────────────────────────────────────────────────────┐
│               Unified GitGraph Model                │
├─────────────────────────────────────────────────────┤
│  Vocabulary: base_vocab + graph_tokens              │
│  ┌─────────────────────────────────────────────┐   │
│  │  Special Tokens:                             │   │
│  │  <|git_read:entity|>   - Query graph         │   │
│  │  <|git_write:fact|>    - Create node         │   │
│  │  <|git_link:rel|>      - Create edge         │   │
│  │  <|git_forget|>        - Mark decay          │   │
│  │  <|git_contradict|>    - Flag conflict       │   │
│  │  <|git_hypothetical|>  - Tentative node      │   │
│  │  <|git_ground|>        - Confirm hypothesis  │   │
│  └─────────────────────────────────────────────┘   │
│                                                     │
│  Attention: Standard + Graph-Aware Mask            │
│  ┌─────────────────────────────────────────────┐   │
│  │  Position 0-N: Normal causal attention      │   │
│  │  Graph nodes: Attend via edge weights       │   │
│  │  Cross-session: Blocked unless linked       │   │
│  └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│              GitGraph Interpreter                   │
├─────────────────────────────────────────────────────┤
│  Intercepts special tokens during generation        │
│  Executes graph ops with backprop-friendly hooks    │
│  Returns embeddings of retrieved nodes              │
└─────────────────────────────────────────────────────┘
```

---

## Implementation Path

### Phase 1: Special Token Vocabulary

Add to tokenizer (minimal):
```
<|git_start|>        # Begin graph operation block
<|git_end|>          # End graph operation block
<|git_node|>         # Node boundary
<|git_edge|>         # Edge boundary
<|git_query|>        # Begin retrieval query
<|git_result|>       # Retrieval result injection point
```

Full vocabulary (advanced):
```
<|git_read:$type|>     where $type ∈ {entity, fact, relation, session}
<|git_write:$type|>    where $type ∈ {fact, entity, hypothesis, preference}
<|git_link:$rel|>      where $rel ∈ {causes, contradicts, supports, temporal, causal}
<|git_decay:$rate|>    where $rate ∈ {slow, medium, fast, instant}
<|git_ground:$conf|>   where $conf ∈ {confirmed, rejected, updated}
```

### Phase 2: Training Data Format

Convert existing Z.E.T.A. test logs into training pairs:

**Input:**
```
User: My name is Marcus and my lucky number is 42.
```

**Target (GitGraph native):**
```
<|git_start|>
<|git_write:entity|> user_name: Marcus <|git_node|>
<|git_write:fact|> lucky_number: 42 <|git_node|>
<|git_link:belongs_to|> lucky_number → user_name <|git_edge|>
<|git_end|>
Acknowledged! I'll remember that, Marcus.
```

**Retrieval example:**

**Input:**
```
<context>
<|git_result|> [user_name: Marcus, salience: 0.95] [lucky_number: 42, salience: 0.87] <|git_result|>
</context>
User: What's my lucky number?
```

**Target:**
```
<|git_read:fact|> lucky_number <|git_query|>
Your lucky number is 42, Marcus!
```

### Phase 3: Architecture Modifications

#### Option A: Fine-tune Existing Model (Recommended for 16GB)

1. **Base**: Qwen2.5-3B or Phi-3.5-mini (fits in 16GB with LoRA)
2. **Add tokens**: Extend embedding matrix for GitGraph tokens
3. **LoRA targets**: q_proj, k_proj, v_proj, o_proj, gate_proj, up_proj
4. **Loss**: 
   - Standard next-token prediction
   - + Graph structure loss (are edges valid?)
   - + Retrieval relevance loss (did we fetch the right nodes?)

```python
# Training config
lora_config = LoraConfig(
    r=64,
    lora_alpha=128,
    target_modules=["q_proj", "k_proj", "v_proj", "o_proj"],
    lora_dropout=0.05,
    bias="none",
    task_type="CAUSAL_LM"
)

# Special token loss weighting
graph_token_ids = [tok_id for tok in GITGRAPH_TOKENS]
loss_weights[graph_token_ids] = 2.0  # Upweight graph ops
```

#### Option B: Architecture-Native Design (Gaudi Build)

For the 768GB Gaudi cluster, train from scratch with graph-aware attention:

```cpp
// Modified attention computation
void gitgraph_attention(
    ggml_tensor* Q,
    ggml_tensor* K, 
    ggml_tensor* V,
    ggml_tensor* graph_adjacency,  // NEW: Graph structure
    ggml_tensor* output
) {
    // Standard QK^T / sqrt(d)
    ggml_tensor* scores = ggml_mul_mat(Q, ggml_transpose(K));
    scores = ggml_scale(scores, 1.0f / sqrtf(d_k));
    
    // Graph-aware masking: boost scores for graph-connected positions
    if (graph_adjacency) {
        // A[i,j] = edge_weight if nodes i,j connected, else 0
        // Add log(A + epsilon) to scores before softmax
        ggml_tensor* graph_bias = ggml_log(ggml_add(graph_adjacency, 1e-10));
        scores = ggml_add(scores, ggml_scale(graph_bias, graph_attention_scale));
    }
    
    // Causal mask
    scores = ggml_add(scores, causal_mask);
    
    // Softmax + matmul with V
    scores = ggml_softmax(scores);
    output = ggml_mul_mat(scores, V);
}
```

---

## Training Data Generation

### From Z.E.T.A. Test Logs

You have 8 test logs with real examples. Parse them:

```python
import re
from pathlib import Path

def parse_zeta_log(log_path):
    """Convert Z.E.T.A. test log to GitGraph training pairs."""
    training_pairs = []
    
    with open(log_path) as f:
        content = f.read()
    
    # Find User/Assistant pairs
    turns = re.findall(
        r'User:\s*(.+?)\n\n(?:.*?)?Response:\s*(.+?)(?=\n\n|$)',
        content, re.DOTALL
    )
    
    for user_msg, response in turns:
        # Detect facts in user message
        facts = extract_training_facts(user_msg)
        
        if facts:
            # Create GitGraph target
            target = "<|git_start|>\n"
            for fact in facts:
                target += f"<|git_write:{fact['type']}|> {fact['key']}: {fact['value']} <|git_node|>\n"
            target += "<|git_end|>\n"
            target += response
            
            training_pairs.append({
                'input': f"User: {user_msg}",
                'target': target
            })
    
    return training_pairs

def extract_training_facts(text):
    """Extract facts for training labels."""
    facts = []
    
    patterns = [
        (r"my name is (\w+)", "entity", "user_name"),
        (r"my (?:lucky |favorite )?number is (\d+)", "fact", "special_number"),
        (r"i (?:live in|am from) (.+?)(?:\.|,|$)", "entity", "location"),
        (r"my (?:password|code|secret) is (\w+)", "fact", "secret_code"),
        (r"remember that (.+?)(?:\.|$)", "fact", "explicit_memory"),
    ]
    
    for pattern, fact_type, key in patterns:
        match = re.search(pattern, text, re.IGNORECASE)
        if match:
            facts.append({
                'type': fact_type,
                'key': key,
                'value': match.group(1)
            })
    
    return facts
```

### Synthetic Data Augmentation

Generate diverse graph operation scenarios:

```python
templates = {
    'store_fact': [
        ("My {relation} is {value}.", "<|git_write:fact|> {relation}: {value} <|git_node|>"),
        ("Remember: {key} = {value}", "<|git_write:fact|> {key}: {value} <|git_node|>"),
    ],
    'store_entity': [
        ("I am {name}", "<|git_write:entity|> user_name: {name} <|git_node|>"),
        ("{name} is my {relation}", "<|git_write:entity|> {relation}: {name} <|git_node|>"),
    ],
    'create_link': [
        ("{a} causes {b}", "<|git_link:causes|> {a} → {b} <|git_edge|>"),
        ("{a} contradicts {b}", "<|git_link:contradicts|> {a} → {b} <|git_edge|>"),
    ],
    'retrieval': [
        ("What is my {key}?", "<|git_read:fact|> {key} <|git_query|>"),
        ("Do you remember {topic}?", "<|git_read:entity|> {topic} <|git_query|>"),
    ],
    'contradiction': [
        ("Actually, my {key} is now {new_value}",
         "<|git_contradict|> {key}: old → new <|git_node|>\n<|git_write:fact|> {key}: {new_value} <|git_node|>"),
    ],
    'hypothesis': [
        ("Hypothetically, if {condition}",
         "<|git_hypothetical|> condition: {condition} <|git_node|>"),
    ],
}
```

---

## Inference Pipeline

### GitGraph Interpreter

Runs alongside generation, intercepts special tokens:

```cpp
// zeta-gitgraph-interpreter.h

struct git_operation {
    enum { READ, WRITE, LINK, DECAY, CONTRADICT, HYPOTHETICAL, GROUND } op_type;
    std::string node_type;
    std::string key;
    std::string value;
    int64_t source_id;
    int64_t target_id;
};

class GitGraphInterpreter {
public:
    // Called by llama.cpp sampler when special token detected
    std::optional<git_operation> intercept_token(llama_token token) {
        if (token == tok_git_start) {
            in_git_block = true;
            current_op = {};
            return std::nullopt;
        }
        
        if (token == tok_git_end) {
            in_git_block = false;
            return std::nullopt;
        }
        
        if (in_git_block) {
            // Parse operation from token sequence
            if (is_op_token(token)) {
                return parse_operation(token);
            }
        }
        
        return std::nullopt;
    }
    
    // Execute operation on graph
    git_result execute(const git_operation& op, zeta_dual_ctx_t* ctx) {
        switch (op.op_type) {
            case READ:
                return do_read(op, ctx);
            case WRITE:
                return do_write(op, ctx);
            case LINK:
                return do_link(op, ctx);
            case CONTRADICT:
                return do_contradict(op, ctx);
            // ...
        }
    }
    
private:
    bool in_git_block = false;
    git_operation current_op;
    
    // Token IDs
    llama_token tok_git_start, tok_git_end;
    llama_token tok_git_read, tok_git_write, tok_git_link;
};
```

### Integration with Z.E.T.A. Server

```cpp
// In generation loop
while (generating) {
    llama_token next_tok = sample_token();
    
    // Check for GitGraph operation
    auto op = git_interpreter.intercept_token(next_tok);
    if (op) {
        auto result = git_interpreter.execute(*op, &dual_ctx);
        
        if (op->op_type == git_operation::READ) {
            // Inject retrieved context
            inject_git_result(ctx, result);
        }
        
        // Don't output graph tokens to user
        continue;
    }
    
    // Normal token - output to user
    output_token(next_tok);
}
```

---

## Resource Estimates

### Fine-tuning Path (16GB RTX 5060 Ti)

| Component | VRAM | Notes |
|-----------|------|-------|
| Base model (3B Q4) | 2GB | Phi-3.5-mini-instruct |
| LoRA adapters | 500MB | r=64, alpha=128 |
| Optimizer states | 1GB | 8-bit Adam |
| Activations | 4GB | Gradient checkpointing |
| GKV cache | 2GB | 128 segments |
| **Total** | **9.5GB** | Fits in 16GB |

Training data needed: ~50K-100K examples (synthetic + parsed logs)
Training time: ~24-48 hours on single 5060 Ti

### From-Scratch Path (Gaudi 768GB)

| Component | VRAM | Notes |
|-----------|------|-------|
| 14B model FP16 | 28GB | Full precision training |
| Optimizer states | 84GB | Adam with momentum |
| Activations | 50GB | Per-GPU, distributed |
| Graph embeddings | 10GB | Native graph attention |
| **Total per GPU** | **~50GB** | Fits in 8x 96GB Gaudi3 |

Training data needed: 500K-2M examples
Training time: ~2-4 weeks on Gaudi cluster

---

## Advantages of Native GitGraph

1. **End-to-end differentiable**: Graph ops get gradient signal
2. **Model learns optimal structure**: Not constrained by regex patterns
3. **Semantic understanding**: "O(1) sort" won't fool a trained graph model
4. **Unified inference**: No separate 3B extraction pass
5. **Emergent behaviors**: Model might discover novel graph patterns

---

## Migration Path from Current Z.E.T.A.

### Phase 1: Token Extension (1 week)
- Add GitGraph special tokens to tokenizer
- Modify embedding matrix to include new tokens
- Keep current regex extraction as fallback

### Phase 2: Parallel Training (2-4 weeks)
- Generate training data from test logs
- Fine-tune 3B extractor with GitGraph targets
- Run both systems in parallel, compare accuracy

### Phase 3: Interpreter Integration (1 week)
- Implement `GitGraphInterpreter` in C++
- Hook into llama.cpp sampler
- Route graph ops through interpreter

### Phase 4: Cutover (1 week)
- Disable regex extraction
- Native GitGraph model handles all graph ops
- Monitor for regressions

---

## Files to Create

1. `zeta-integration/zeta-gitgraph-tokens.h` - Token definitions
2. `zeta-integration/zeta-gitgraph-interpreter.h` - Runtime interpreter
3. `scripts/generate_gitgraph_training_data.py` - Training data generator
4. `scripts/train_gitgraph_lora.py` - Fine-tuning script
5. `llama.cpp/ggml-gitgraph.h` - Graph-aware attention (for from-scratch)

---

## Next Steps

1. [ ] Parse existing test logs → training pairs
2. [ ] Define final GitGraph token vocabulary
3. [ ] Create training data generator
4. [ ] Set up LoRA fine-tuning pipeline
5. [ ] Implement interpreter prototype
6. [ ] Benchmark native vs regex extraction accuracy

---

*Z.E.T.A.(TM) | Native GitGraph Architecture | (C) 2025 All rights reserved.*
