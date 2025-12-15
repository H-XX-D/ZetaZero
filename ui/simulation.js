// Z.E.T.A. Live Inference Visualization
// Phase 3: HoloGit Tree of Thought

const canvas = document.getElementById('zetaCanvas');
const ctx = canvas.getContext('2d');

let width, height;
let cameraY = 0;
let targetCameraY = 0;

// --- CONFIGURATION ---
const COLORS = {
    bg: '#0a0a0a',
    primary: '#ff003c',   // Red (Active/Hot)
    accent: '#fcee0a',    // Yellow (Math/Warning)
    secondary: '#00f0ff', // Cyan (Data/Cold)
    breakthrough: '#00ff9d', // Green (Success)
    deadend: '#555555',   // Grey (Failed path)
    dim: '#333333',
    text: '#e0e0e0'
};

// --- MATH EQUATIONS ---
const MATH_SNIPPETS = [
    "iℏ ∂ψ/∂t = Ĥψ",
    "∇ × E = -∂B/∂t",
    "R_μν - ½Rg_μν = 8πGT_μν",
    "∫ e^(-x²) dx = √π",
    "e^(iπ) + 1 = 0"
];

// --- SCRIPT & TRIGGERS ---
const SCRIPT = [
    { speaker: ">X<", text: "INITIATING_HOLOGIT_VISUALIZATION...", trigger: "root" },
    { speaker: "ZERO", text: "MEMORY_MODULES_ONLINE.", trigger: "branch_1" },
    { speaker: ">X<", text: "Let's map the chaos. Start with the noise.", trigger: "noise_nodes" },
    { speaker: "ZERO", text: "ENTROPY_HIGH._CALCULATING_PROBABILITIES.", trigger: "math_1" },
    { speaker: ">X<", text: "Filter for coherent signals. Prune the dead ends.", trigger: "prune" },
    { speaker: "ZERO", text: "OPTIMIZING_PATH._LINEAR_ALGEBRA_APPLIED.", trigger: "math_2" },
    { speaker: ">X<", text: "There. That branch. The decoherence model.", trigger: "branch_2" },
    { speaker: "ZERO", text: "SIMULATING_DECAY_FUNCTIONS...", trigger: "math_3" },
    { speaker: ">X<", text: "It's failing. That path is a dead end.", trigger: "dead_end" },
    { speaker: "ZERO", text: "REROUTING._TRYING_QUANTUM_TUNNELING.", trigger: "branch_3" },
    { speaker: ">X<", text: "Wait... the equations are aligning.", trigger: "converge_start" },
    { speaker: "ZERO", text: "FUNDAMENTAL_BREAKTHROUGH_IMMINENT.", trigger: "math_4" },
    { speaker: ">X<", text: "LOCK IT IN! CONVERGE ALL STREAMS!", trigger: "breakthrough" },
    { speaker: "ZERO", text: "INTEGRATION_COMPLETE._COLD_FUSION_ACHIEVED.", trigger: "final" }
];

// --- STATE ---
let scriptState = {
    lineIndex: 0,
    charIndex: 0,
    currentText: "",
    isWaiting: false,
    waitTimer: 0
};

let nodes = [];
let pulses = [];
let frame = 0;

// --- CLASSES ---

class HoloNode {
    constructor(x, y, type, label, parent = null) {
        this.x = x;
        this.y = y;
        this.type = type; // 'root', 'normal', 'math', 'deadend', 'breakthrough'
        this.label = label;
        this.parent = parent;
        this.children = [];
        this.age = 0;
        this.scale = 0;
        
        if (parent) {
            parent.children.push(this);
        }
    }

    update() {
        this.age++;
        if (this.scale < 1) this.scale += 0.05;
    }

    draw() {
        if (this.scale <= 0.01) return;

        // Draw Connection to Parent
        if (this.parent) {
            ctx.beginPath();
            ctx.moveTo(this.parent.x, this.parent.y);
            
            // Circuit-style or Bezier? Let's go Bezier for "organic" feel
            const midY = (this.parent.y + this.y) / 2;
            ctx.bezierCurveTo(this.parent.x, midY, this.x, midY, this.x, this.y);
            
            ctx.strokeStyle = this.type === 'deadend' ? COLORS.deadend : COLORS.secondary;
            if (this.type === 'breakthrough') ctx.strokeStyle = COLORS.breakthrough;
            
            ctx.lineWidth = (this.type === 'breakthrough' ? 4 : 2) * this.scale;
            ctx.stroke();
        }

        // Draw Node Body
        ctx.save();
        ctx.translate(this.x, this.y);
        ctx.scale(this.scale, this.scale);

        let color = COLORS.secondary;
        if (this.type === 'math') color = COLORS.accent;
        if (this.type === 'deadend') color = COLORS.deadend;
        if (this.type === 'breakthrough') color = COLORS.breakthrough;
        if (this.type === 'root') color = COLORS.primary;

        // Glow
        if (this.type !== 'deadend') {
            ctx.shadowBlur = 15;
            ctx.shadowColor = color;
        }

        // Shape
        ctx.fillStyle = COLORS.bg;
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;

        if (this.type === 'math') {
            ctx.strokeRect(-60, -20, 120, 40);
            ctx.fillStyle = color;
            ctx.font = '14px "Share Tech Mono"';
            ctx.textAlign = 'center';
            ctx.fillText(this.label, 0, 5);
        } else {
            ctx.beginPath();
            ctx.arc(0, 0, 10, 0, Math.PI * 2);
            ctx.fill();
            ctx.stroke();
            
            // Label
            ctx.fillStyle = color;
            ctx.font = '12px "Share Tech Mono"';
            ctx.textAlign = 'left';
            ctx.fillText(this.label, 15, 4);
        }

        ctx.restore();
    }
}

class Pulse {
    constructor(startNode, endNode) {
        this.startNode = startNode;
        this.endNode = endNode;
        this.progress = 0;
        this.speed = 0.02 + Math.random() * 0.03;
        this.color = COLORS.secondary;
        if (endNode.type === 'breakthrough') this.color = COLORS.breakthrough;
        if (endNode.type === 'math') this.color = COLORS.accent;
    }

    update() {
        this.progress += this.speed;
        return this.progress < 1;
    }

    draw() {
        if (!this.startNode || !this.endNode) return;
        
        // Calculate position on bezier curve
        const t = this.progress;
        const p0 = {x: this.startNode.x, y: this.startNode.y};
        const p1 = {x: this.endNode.x, y: this.endNode.y};
        
        // Simple interpolation for now, ideally should match the bezier control points of the node connection
        const midY = (p0.y + p1.y) / 2;
        
        // Bezier formula: (1-t)^3*P0 + 3(1-t)^2*t*P1 + 3(1-t)*t^2*P2 + t^3*P3
        // Here we used a quadratic-ish bezier in node draw: P0 -> (P0.x, midY) -> (P1.x, midY) -> P1
        // Let's approximate
        
        const cx = (1-t)*(1-t)*p0.x + 2*(1-t)*t*((p0.x+p1.x)/2) + t*t*p1.x; // Simplified
        // Actually let's just lerp for the pulse, it's fast enough
        
        // Better Bezier calc
        const cp1 = {x: p0.x, y: midY};
        const cp2 = {x: p1.x, y: midY};
        
        const x = Math.pow(1-t, 3)*p0.x + 3*Math.pow(1-t, 2)*t*cp1.x + 3*(1-t)*Math.pow(t, 2)*cp2.x + Math.pow(t, 3)*p1.x;
        const y = Math.pow(1-t, 3)*p0.y + 3*Math.pow(1-t, 2)*t*cp1.y + 3*(1-t)*Math.pow(t, 2)*cp2.y + Math.pow(t, 3)*p1.y;

        ctx.fillStyle = this.color;
        ctx.shadowBlur = 10;
        ctx.shadowColor = this.color;
        ctx.beginPath();
        ctx.arc(x, y, 3, 0, Math.PI*2);
        ctx.fill();
        ctx.shadowBlur = 0;
    }
}

// --- INIT ---

function init() {
    resize();
    window.addEventListener('resize', resize);
    animate();
}

function resize() {
    width = canvas.width = canvas.parentElement.clientWidth;
    height = canvas.height = canvas.parentElement.clientHeight;
}

// --- LOGIC ---

function handleTrigger(trigger) {
    const lastNode = nodes.length > 0 ? nodes[nodes.length - 1] : null;
    const centerX = width / 2;
    const spacingY = 600; // Vertical spacing (Increased as requested)

    if (trigger === "root") {
        nodes.push(new HoloNode(centerX, 100, 'root', 'ROOT: CHAOS'));
    }
    else if (trigger === "branch_1") {
        const root = nodes[0];
        nodes.push(new HoloNode(centerX - 100, root.y + spacingY, 'normal', 'MEM_MOD_A', root));
        nodes.push(new HoloNode(centerX + 100, root.y + spacingY, 'normal', 'MEM_MOD_B', root));
    }
    else if (trigger === "noise_nodes") {
        const parents = nodes.slice(-2);
        parents.forEach(p => {
            nodes.push(new HoloNode(p.x + (Math.random()*60 - 30), p.y + spacingY, 'normal', 'NOISE_FILTER', p));
        });
    }
    else if (trigger === "math_1") {
        const parent = nodes[nodes.length - 1];
        nodes.push(new HoloNode(centerX, parent.y + spacingY, 'math', MATH_SNIPPETS[0], parent));
    }
    else if (trigger === "prune") {
        // No new nodes, just visual effect maybe?
    }
    else if (trigger === "math_2") {
        const parent = nodes.filter(n => n.type === 'math')[0];
        nodes.push(new HoloNode(centerX - 150, parent.y + spacingY, 'math', MATH_SNIPPETS[1], parent));
        nodes.push(new HoloNode(centerX + 150, parent.y + spacingY, 'normal', 'PATH_OPTIMAL', parent));
    }
    else if (trigger === "branch_2") {
        const p1 = nodes[nodes.length - 2]; // Math node
        const p2 = nodes[nodes.length - 1]; // Normal node
        nodes.push(new HoloNode(p1.x, p1.y + spacingY, 'normal', 'DECOHERENCE_SIM', p1));
        nodes.push(new HoloNode(p2.x, p2.y + spacingY, 'normal', 'STABILITY_CHECK', p2));
    }
    else if (trigger === "math_3") {
        const p = nodes[nodes.length - 2]; // Decoherence
        nodes.push(new HoloNode(p.x, p.y + spacingY, 'math', MATH_SNIPPETS[3], p));
    }
    else if (trigger === "dead_end") {
        const p = nodes[nodes.length - 1];
        nodes.push(new HoloNode(p.x, p.y + 100, 'deadend', 'COLLAPSE', p));
    }
    else if (trigger === "branch_3") {
        const p = nodes.find(n => n.label === 'STABILITY_CHECK');
        nodes.push(new HoloNode(p.x, p.y + spacingY * 2, 'normal', 'TUNNELING_PROTOCOL', p));
    }
    else if (trigger === "converge_start") {
        const p = nodes[nodes.length - 1];
        nodes.push(new HoloNode(centerX, p.y + spacingY, 'math', MATH_SNIPPETS[4], p));
    }
    else if (trigger === "math_4") {
        const p = nodes[nodes.length - 1];
        nodes.push(new HoloNode(centerX, p.y + spacingY, 'breakthrough', 'BREAKTHROUGH', p));
    }
    else if (trigger === "breakthrough") {
        // Visual flare
    }
    else if (trigger === "final") {
        const p = nodes[nodes.length - 1];
        nodes.push(new HoloNode(centerX, p.y + spacingY, 'breakthrough', 'COLD_FUSION_CORE', p));
    }

    // Auto-scroll to latest node
    if (nodes.length > 0) {
        const last = nodes[nodes.length - 1];
        targetCameraY = last.y - height / 2;
    }
}

function updateScript() {
    if (scriptState.isWaiting) {
        scriptState.waitTimer++;
        if (scriptState.waitTimer > 120) { // Wait time between lines
            scriptState.isWaiting = false;
            scriptState.waitTimer = 0;
            scriptState.lineIndex++;
            
            if (scriptState.lineIndex < SCRIPT.length) {
                scriptState.charIndex = 0;
                scriptState.currentText = "";
                // Fire trigger for NEW line
                if (SCRIPT[scriptState.lineIndex].trigger) {
                    handleTrigger(SCRIPT[scriptState.lineIndex].trigger);
                }
            }
        }
        return;
    }

    if (scriptState.lineIndex >= SCRIPT.length) return;

    const line = SCRIPT[scriptState.lineIndex];
    const fullLine = `${line.speaker}: ${line.text}`;

    if (scriptState.charIndex < fullLine.length) {
        scriptState.currentText += fullLine[scriptState.charIndex];
        scriptState.charIndex++;
    } else {
        scriptState.isWaiting = true;
    }
}

function spawnPulses() {
    if (Math.random() > 0.85) {
        // Find a random connection
        const connectedNodes = nodes.filter(n => n.parent);
        if (connectedNodes.length > 0) {
            const target = connectedNodes[Math.floor(Math.random() * connectedNodes.length)];
            
            // Randomize direction: Down (Parent -> Child) or Up (Child -> Parent)
            if (Math.random() > 0.5) {
                pulses.push(new Pulse(target.parent, target)); // Down
            } else {
                pulses.push(new Pulse(target, target.parent)); // Up
            }
        }
    }
}

function draw() {
    frame++;
    
    // Camera Smooth Scroll
    cameraY += (targetCameraY - cameraY) * 0.05;

    ctx.fillStyle = COLORS.bg;
    ctx.fillRect(0, 0, width, height);

    ctx.save();
    ctx.translate(0, -cameraY);

    // Draw Nodes & Pulses
    nodes.forEach(n => n.update());
    nodes.forEach(n => n.draw());
    
    pulses = pulses.filter(p => p.update());
    pulses.forEach(p => p.draw());

    ctx.restore();
    
    // Draw Script Overlay (Fixed Position)
    drawScriptOverlay();
}

function drawScriptOverlay() {
    if (scriptState.lineIndex >= SCRIPT.length && !scriptState.currentText) return;

    const line = SCRIPT[Math.min(scriptState.lineIndex, SCRIPT.length - 1)];
    
    // Background for text
    ctx.fillStyle = 'rgba(10, 10, 10, 0.8)';
    ctx.fillRect(0, height - 100, width, 100);
    ctx.strokeStyle = COLORS.dim;
    ctx.beginPath();
    ctx.moveTo(0, height - 100);
    ctx.lineTo(width, height - 100);
    ctx.stroke();

    // Text
    ctx.font = '20px "Share Tech Mono"';
    ctx.textAlign = 'left';
    
    let textColor = COLORS.text;
    if (line.speaker === "ZERO") textColor = COLORS.secondary;
    if (line.speaker === ">X<") textColor = COLORS.primary;

    ctx.fillStyle = textColor;
    ctx.shadowBlur = 10;
    ctx.shadowColor = textColor;
    
    ctx.fillText(scriptState.currentText, 50, height - 45);
    ctx.shadowBlur = 0;
}

function animate() {
    updateScript();
    spawnPulses();
    draw();
    requestAnimationFrame(animate);
}

init();
