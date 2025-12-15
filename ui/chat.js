// Knowledge Base
const KNOWLEDGE_BASE = {
    "entanglement": {
        title: "Entanglement (Sharpened Cosine Similarity)",
        math: "E(q, s) = ReLU( (q · s) / (|q| |s|) )³",
        code: `// Metal Kernel Implementation
float similarity = dot(q, s) / (length(q) * length(s));
float entanglement = max(0.0, pow(similarity, 3.0));
return entanglement;`
    },
    "sublimation": {
        title: "Sublimation (Hierarchical Compression)",
        math: "W_active → Serialize → s ∈ R^d",
        code: `// Swift Implementation
func sublimate(tokens: [Int], kvData: Data) {
    let vector = computeMeanPooledVector(kvData)
    let id = UUID()
    diskStorage.write(id, data: kvData)
    ramIndex.add(id, vector: vector)
}`
    },
    "decay": {
        title: "Zeta Potential Decay",
        math: "Z(t) = Z₀ · e^(-λ(t - t_recall))",
        code: `// Temporal Decay Logic
let timeDelta = currentTime - recallTime
let zetaPotential = initialEnergy * exp(-lambda * timeDelta)
if zetaPotential < 0.01 { removeSuperposition(id) }`
    },
    "tunneling": {
        title: "Tunneling (Sparse Attention Gating)",
        math: "T(a_ij) = -∞ if a_ij ≤ τ",
        code: `// Sparse Masking
if (attentionScore <= tunnelingThreshold) {
    return -INFINITY; // Gate closed
} else {
    return attentionScore; // Tunnel open
}`
    },
    "superposition": {
        title: "Resonant Superposition",
        math: "O_unified = Attn_active + Σ Z_k · Attn_memory^k",
        code: `// Unified Attention Loop
var output = standardAttention(q, k, v)
for memory in activeSuperpositions {
    let z = memory.zetaPotential
    output += z * tunnelingAttention(q, memory.k, memory.v)
}`
    }
};

// Chat UI Logic
const chatToggle = document.getElementById('chat-toggle');
const chatWindow = document.getElementById('chat-window');
const chatClose = document.getElementById('chat-close');
const chatInput = document.getElementById('chat-input');
const chatSend = document.getElementById('chat-send');
const chatMessages = document.getElementById('chat-messages');

chatToggle.addEventListener('click', () => {
    chatWindow.classList.remove('hidden');
});

chatClose.addEventListener('click', () => {
    chatWindow.classList.add('hidden');
});

function addMessage(content, type) {
    const div = document.createElement('div');
    div.className = `message ${type}`;
    div.innerHTML = content;
    chatMessages.appendChild(div);
    chatMessages.scrollTop = chatMessages.scrollHeight;
}

// HUD Logic
const holoProjection = document.getElementById('holo-projection');
const holoContent = document.getElementById('holo-content');
const holoClose = document.getElementById('holo-close');

holoClose.addEventListener('click', () => {
    holoProjection.classList.add('hidden');
});

function updateHoloHUD(match) {
    holoContent.innerHTML = `
        <div class="term-title">${match.title}</div>
        <div class="math-block">${match.math}</div>
        <div class="code-block">${match.code}</div>
    `;
    holoProjection.classList.remove('hidden');
}

function handleQuery() {
    const term = chatInput.value.trim().toLowerCase();
    if (!term) return;

    addMessage(term, 'user');
    chatInput.value = '';

    // Search Logic
    let match = null;
    for (const key in KNOWLEDGE_BASE) {
        if (term.includes(key) || key.includes(term)) {
            match = KNOWLEDGE_BASE[key];
            break;
        }
    }

    if (match) {
        const response = `
            <div class="term-title">${match.title}</div>
            <div class="math-block">${match.math}</div>
            <div class="code-block">${match.code}</div>
        `;
        setTimeout(() => {
            addMessage(response, 'bot');
            updateHoloHUD(match); // Trigger HUD
        }, 500);
    } else {
        // Simulate Memory Formation
        const response = `
            <div class="term-title">New Concept Detected: "${term}"</div>
            <div class="message system">
                <p>⚠️ Resonance Failed (No Entanglement).</p>
                <p>🔄 Initiating <strong>Sublimation</strong> sequence...</p>
                <p>💾 Committing to <strong>HoloGit</strong>...</p>
                <p class="hint">Correlation convergence pending (v0.1 -> v0.2)</p>
            </div>
        `;
        setTimeout(() => addMessage(response, 'bot'), 500);
    }
}

chatSend.addEventListener('click', handleQuery);
chatInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') handleQuery();
});
