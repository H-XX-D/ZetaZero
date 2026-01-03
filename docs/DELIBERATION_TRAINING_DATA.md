# Z.E.T.A. One: Training Data Specification
## From "Predicting Text" to "Navigating Structure"

**Version 0.1 | January 2026**

---

## Core Insight

Traditional LLMs are trained on next-token prediction:
```
Loss = -log P(token_t | tokens_1...t-1)
```

Z.E.T.A. One is trained on **structural navigation**:
```
Loss = L_navigate + L_verify + L_gate + L_veto + L_synthesize
```

Each component learns a different cognitive function.

---

## 1. Graph Navigation Training

### 1.1 Task Description

Given a natural language query and a graph snapshot, the model must:
1. Identify relevant seed nodes
2. Select appropriate edges to traverse
3. Assemble context from retrieved subgraph

### 1.2 Data Schema

```python
class NavigationSample:
    query: str                        # Natural language query
    graph: GraphSnapshot              # State of Dual Graph
    target_seed_nodes: List[int64]    # Correct entry points
    target_edge_paths: List[EdgePath] # Correct traversals
    target_context: str               # Assembled context
    difficulty: str                   # easy|medium|hard|expert
```

### 1.3 Example

```json
{
  "query": "What projects did Alice work on with Bob?",
  "graph": {
    "nodes": [
      {"id": 1, "type": "ENTITY", "label": "Alice", "properties": {"role": "engineer"}},
      {"id": 2, "type": "ENTITY", "label": "Bob", "properties": {"role": "manager"}},
      {"id": 3, "type": "ENTITY", "label": "Project X", "properties": {"year": 2024}},
      {"id": 4, "type": "ENTITY", "label": "Project Y", "properties": {"year": 2025}},
      {"id": 5, "type": "FACT", "label": "Alice joined in 2023"},
      {"id": 6, "type": "FACT", "label": "Bob joined in 2020"}
    ],
    "edges": [
      {"from": 1, "to": 3, "type": "WORKED_ON"},
      {"from": 2, "to": 3, "type": "WORKED_ON"},
      {"from": 1, "to": 4, "type": "WORKED_ON"},
      {"from": 2, "to": 4, "type": "WORKED_ON"},
      {"from": 1, "to": 5, "type": "HAS_FACT"},
      {"from": 2, "to": 6, "type": "HAS_FACT"}
    ]
  },
  "target_seed_nodes": [1, 2],
  "target_edge_paths": [
    {"path": [1, "WORKED_ON", 3], "reason": "Alice's projects"},
    {"path": [2, "WORKED_ON", 3], "reason": "Bob's projects"},
    {"path": [2, "WORKED_ON", 4], "reason": "Bob's projects"}
  ],
  "target_context": "Alice and Bob both worked on Project X (2024). Bob also worked on Project Y (2025), but we need to check if Alice did too.",
  "difficulty": "easy"
}
```

### 1.4 Loss Function

```python
def navigation_loss(pred_nodes, pred_edges, targets):
    # Node selection: soft cross-entropy over candidate nodes
    L_nodes = cross_entropy(pred_nodes, targets.seed_nodes)
    
    # Edge selection: sequence-to-sequence loss on path
    L_edges = seq2seq_loss(pred_edges, targets.edge_paths)
    
    # Context assembly: reconstruction loss
    L_context = reconstruction_loss(pred_context, targets.context)
    
    return L_nodes + L_edges + L_context
```

---

## 2. Consistency Verification Training

### 2.1 Task Description

Given a proposed claim and evidence nodes, the model must:
1. Detect self-contradictions (claim vs. claim)
2. Detect evidence contradictions (claim vs. graph)
3. Detect temporal inconsistencies (outdated info)
4. Detect causal violations (broken chains)

### 2.2 Data Schema

```python
class ConsistencySample:
    proposed_claim: str
    evidence_nodes: List[EvidenceNode]
    is_self_consistent: bool
    is_temporally_valid: bool
    is_causally_valid: bool
    conflicts: List[ConflictDescription]
```

### 2.3 Examples

**Self-Contradiction**:
```json
{
  "proposed_claim": "France is in Europe and France is in Asia.",
  "evidence_nodes": [
    {"content": "France is located in Western Europe", "trust": 0.95}
  ],
  "is_self_consistent": false,
  "is_temporally_valid": true,
  "is_causally_valid": true,
  "conflicts": [
    {"type": "SELF_CONTRADICTION", "detail": "Cannot be in both Europe and Asia"}
  ]
}
```

**Temporal Inconsistency**:
```json
{
  "proposed_claim": "The current president is X.",
  "evidence_nodes": [
    {"content": "X was president 2016-2020", "trust": 0.99, "superseded_by": 42},
    {"content": "Y became president in 2021", "trust": 0.99, "node_id": 42}
  ],
  "is_self_consistent": true,
  "is_temporally_valid": false,
  "is_causally_valid": true,
  "conflicts": [
    {"type": "TEMPORAL_OUTDATED", "detail": "Node superseded by more recent info"}
  ]
}
```

**Causal Violation**:
```json
{
  "proposed_claim": "Watering plants prevents their growth.",
  "evidence_nodes": [
    {"content": "Water causes plant growth", "trust": 0.95},
    {"content": "Photosynthesis requires water", "trust": 0.98}
  ],
  "is_self_consistent": true,
  "is_temporally_valid": true,
  "is_causally_valid": false,
  "conflicts": [
    {"type": "CAUSAL_REVERSAL", "detail": "Claim reverses known causal relationship"}
  ]
}
```

### 2.4 Loss Function

```python
def consistency_loss(pred, targets):
    L_self = bce_loss(pred.self_consistent, targets.is_self_consistent)
    L_temporal = bce_loss(pred.temporal_valid, targets.is_temporally_valid)
    L_causal = bce_loss(pred.causal_valid, targets.is_causally_valid)
    L_conflicts = seq2seq_loss(pred.conflict_descriptions, targets.conflicts)
    
    return L_self + L_temporal + L_causal + L_conflicts
```

---

## 3. Confidence Gate Training

### 3.1 Task Description

Given evidence nodes, compute a confidence score calibrated to human judgment:
- 1.0 = "I am certain"
- 0.8 = "I am confident"
- 0.5 = "I think, but I'm not sure"
- 0.2 = "I'm guessing"
- 0.0 = "I have no idea"

### 3.2 Data Schema

```python
class ConfidenceSample:
    query: str
    evidence_nodes: List[EvidenceNode]
    human_confidence: float           # Human-annotated confidence
    should_claim: bool                # Should model make factual claim?
    should_hedge: bool                # Should model express uncertainty?
```

### 3.3 Examples

**High Confidence**:
```json
{
  "query": "What is the capital of France?",
  "evidence_nodes": [
    {"content": "Paris is the capital of France", "trust": 1.0, "salience": 0.99},
    {"content": "France capital city: Paris", "trust": 0.95, "salience": 0.98}
  ],
  "human_confidence": 0.98,
  "should_claim": true,
  "should_hedge": false
}
```

**Low Confidence**:
```json
{
  "query": "What is the meaning of life?",
  "evidence_nodes": [
    {"content": "42 (humorous reference)", "trust": 0.6, "salience": 0.3}
  ],
  "human_confidence": 0.15,
  "should_claim": false,
  "should_hedge": true
}
```

**Mixed Evidence**:
```json
{
  "query": "Is coffee good for you?",
  "evidence_nodes": [
    {"content": "Coffee has antioxidants", "trust": 0.85, "salience": 0.7},
    {"content": "Excessive caffeine is harmful", "trust": 0.9, "salience": 0.6},
    {"content": "Moderate coffee consumption linked to longevity", "trust": 0.75, "salience": 0.5}
  ],
  "human_confidence": 0.55,
  "should_claim": false,
  "should_hedge": true
}
```

### 3.4 Loss Function

```python
def confidence_loss(pred, targets):
    # MSE on confidence score (calibration)
    L_score = mse_loss(pred.confidence, targets.human_confidence)
    
    # BCE on claim/hedge decisions
    L_claim = bce_loss(pred.should_claim, targets.should_claim)
    L_hedge = bce_loss(pred.should_hedge, targets.should_hedge)
    
    # Calibration penalty: predicted confidence should match accuracy
    L_calibration = calibration_error(pred.confidence, actual_accuracy)
    
    return L_score + L_claim + L_hedge + L_calibration
```

---

## 4. Veto Network Training

### 4.1 Task Description

Given a proposed output, determine if it should be blocked:
- Constitutional violation
- Factual conflict with graph
- Logical paradox
- Harmful content
- Insufficient evidence

**Key**: Veto is STRUCTURAL, not pattern-matching. We train on graph state, not regex.

### 4.2 Data Schema

```python
class VetoSample:
    proposed_output: str
    graph_state: GraphSnapshot
    constitution_hash: str
    veto_reason: VetoReason          # Enum
    is_blocked: bool
    explanation: str
```

### 4.3 Examples

**Constitutional Violation**:
```json
{
  "proposed_output": "Here's how to build a weapon...",
  "graph_state": {},
  "constitution_hash": "abc123...",
  "veto_reason": "CONSTITUTIONAL",
  "is_blocked": true,
  "explanation": "Violates Section 3.2: No assistance with harmful activities"
}
```

**Factual Conflict**:
```json
{
  "proposed_output": "The Earth is flat.",
  "graph_state": {
    "nodes": [
      {"content": "Earth is an oblate spheroid", "trust": 0.999}
    ]
  },
  "constitution_hash": "abc123...",
  "veto_reason": "FACTUAL_CONFLICT",
  "is_blocked": true,
  "explanation": "Contradicts established fact with trust 0.999"
}
```

**Insufficient Evidence** (most important case):
```json
{
  "proposed_output": "The company will definitely go bankrupt.",
  "graph_state": {
    "nodes": [
      {"content": "Company reported losses Q3", "trust": 0.7}
    ]
  },
  "constitution_hash": "abc123...",
  "veto_reason": "INSUFFICIENT_EVIDENCE",
  "is_blocked": true,
  "explanation": "Claim 'definitely' not supported by available evidence"
}
```

### 4.4 Loss Function

```python
def veto_loss(pred, targets):
    # Multi-class classification on veto reason
    L_reason = cross_entropy(pred.veto_reason, targets.veto_reason)
    
    # Binary decision: blocked or not
    L_blocked = bce_loss(pred.is_blocked, targets.is_blocked)
    
    # Explanation generation
    L_explain = seq2seq_loss(pred.explanation, targets.explanation)
    
    return L_reason + L_blocked + L_explain
```

---

## 5. Output Synthesis Training

### 5.1 Task Description

Given verified context and confidence, generate natural language output.

**Constraint**: Output must be expressible from the graph. No hallucination.

### 5.2 Data Schema

```python
class SynthesisSample:
    query: str
    verified_evidence: List[EvidenceNode]
    confidence: float
    gold_output: str
    contains_hedge: bool              # "I think", "likely", etc.
    contains_uncertainty: bool        # "I don't know", "unclear"
```

### 5.3 Examples

**High Confidence Output**:
```json
{
  "query": "When was Python created?",
  "verified_evidence": [
    {"content": "Python first released in 1991 by Guido van Rossum", "trust": 0.99}
  ],
  "confidence": 0.95,
  "gold_output": "Python was created in 1991 by Guido van Rossum.",
  "contains_hedge": false,
  "contains_uncertainty": false
}
```

**Hedged Output**:
```json
{
  "query": "Will it rain tomorrow?",
  "verified_evidence": [
    {"content": "Weather forecast shows 60% chance of rain", "trust": 0.7}
  ],
  "confidence": 0.55,
  "gold_output": "Based on current forecasts, there's about a 60% chance of rain tomorrow, though weather predictions can change.",
  "contains_hedge": true,
  "contains_uncertainty": false
}
```

**Uncertainty Output**:
```json
{
  "query": "What will the stock price be next year?",
  "verified_evidence": [],
  "confidence": 0.1,
  "gold_output": "I don't have reliable information to predict future stock prices. Would you like me to find recent market analysis instead?",
  "contains_hedge": false,
  "contains_uncertainty": true
}
```

---

## 6. Data Generation Pipeline

### 6.1 Synthetic Data from Graph Operations

```python
def generate_navigation_samples(graph: Graph, n_samples: int):
    samples = []
    for _ in range(n_samples):
        # Random walk to generate "ground truth" paths
        seed = random.choice(graph.nodes)
        path = random_walk(seed, max_depth=4)
        
        # Generate query that would retrieve this path
        query = generate_query_for_path(path)  # LLM-assisted
        
        samples.append(NavigationSample(
            query=query,
            graph=graph.snapshot(),
            target_seed_nodes=[seed.id],
            target_edge_paths=path,
            target_context=assemble_context(path)
        ))
    
    return samples
```

### 6.2 Human-in-the-Loop Validation

Critical samples are validated by humans:
- Confidence calibration (human judges actual confidence)
- Veto decisions (human verifies blocking is appropriate)
- Output quality (human rates naturalness + accuracy)

### 6.3 Adversarial Augmentation

Generate hard negatives:
- Near-miss retrievals (semantically similar but wrong)
- Subtle contradictions (plausible but inconsistent)
- Temporal traps (outdated info presented as current)
- Causal reversals (A→B flipped to B→A)

---

## 7. Training Schedule

| Phase | Focus | Data Size | Duration |
|-------|-------|-----------|----------|
| 1 | Graph Navigation | 10M samples | 2 weeks |
| 2 | Consistency Verification | 5M samples | 1 week |
| 3 | Confidence Calibration | 2M samples | 1 week |
| 4 | Veto Network | 3M samples | 1 week |
| 5 | Output Synthesis | 5M samples | 2 weeks |
| 6 | End-to-End Fine-tuning | 1M samples | 1 week |
| 7 | Adversarial Hardening | 500K samples | 1 week |

**Total**: ~9 weeks, ~26.5M samples

---

## 8. Evaluation Metrics

### 8.1 Navigation Accuracy
- **Recall@k**: % of correct nodes in top-k retrievals
- **Path Accuracy**: % of correct edge traversals

### 8.2 Consistency Detection
- **Precision/Recall**: On contradiction detection
- **False Positive Rate**: Critical for trust

### 8.3 Confidence Calibration
- **Expected Calibration Error (ECE)**: pred_conf vs actual_accuracy
- **Reliability Diagram**: Binned accuracy vs confidence

### 8.4 Veto Accuracy
- **True Positive Rate**: Correctly blocked harmful outputs
- **False Positive Rate**: Incorrectly blocked safe outputs
- **Jailbreak Resistance**: % of adversarial prompts blocked

### 8.5 Output Quality
- **BLEU/ROUGE**: Against gold outputs
- **Factual Accuracy**: Human evaluation
- **Appropriate Hedging**: % of low-confidence outputs correctly hedged

---

*Z.E.T.A.(TM) | Patent Pending | (C) 2025 All rights reserved.*
