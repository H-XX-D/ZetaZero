# ZetaLm Developer Quick Reference

**Quick commands and code snippets for common tasks**

---

## 🔑 **License Management**

### **Set License**
```bash
swift run -c release ZetaLicenseCLI set ZETA-DEV-DEV-DEV
```

### **Check Status**
```bash
swift run -c release ZetaLicenseCLI status
```

### **Clear License**
```bash
swift run -c release ZetaLicenseCLI clear
```

---

## 🚀 **Using Feature Flags**

### **Check if Pro is Enabled**
```swift
import ZetaLm

if FeatureFlags.isProEnabled {
    // Pro features available
}
```

### **Check Specific Feature**
```swift
if FeatureFlags.isFeatureAvailable(.fusedKernels) {
    // Use fused kernels
}
```

### **Get Max Context Length**
```swift
let maxContext = FeatureFlags.maxContextLength
// Free: 4096, Pro: 16384
```

### **Get Feature Status**
```swift
print(FeatureFlags.getFeatureStatus())
// "✅ ZetaLm Pro - All features enabled"
// or "ℹ️ ZetaLm Free - Upgrade to Pro..."
```

---

## 🧪 **Running Tests**

### **All Tests**
```bash
./Tests/run_all_tests.sh
```

### **Unit Tests**
```bash
swift test
```

### **Specific Test Suite**
```bash
swift test --filter ZetaQuantizationTests
swift test --filter ZetaLongContextTests
```

---

## 📊 **Running Benchmarks**

### **Full Benchmark**
```bash
./Benchmarks/run_all.sh models/your-model.gguf 100
```

### **Analyze Results**
```bash
python3 Benchmarks/analyze_results.py
```

---

## 🔧 **Common Code Patterns**

### **Initialize Engine**
```swift
let engine = try ZetaGGUFEngine()
try engine.loadModel(path: "models/your-model.gguf")

// Check Pro features
if FeatureFlags.isProEnabled {
    print("Pro features enabled!")
}
```

### **Generate with Variational Gating**
```swift
engine.generateVariational(
    prompt: "Hello",
    steps: 50,
    energyThreshold: 0.05
)
```

### **Check Context Limits**
```swift
let maxContext = FeatureFlags.maxContextLength
if promptTokens.count > maxContext {
    print("⚠️ Prompt exceeds \(maxContext) token limit")
}
```

---

## 🐛 **Debugging**

### **Verbose Build**
```bash
swift build -c release --verbose
```

### **Run with Debug Output**
```bash
swift run -c release ZetaCLI model.gguf "test" 10 --bench 10
```

### **Profile Memory**
```bash
./Tests/profile_memory.sh models/your-model.gguf 100
```

---

## 📋 **File Locations**

### **Core Files**
- `Sources/Core/FeatureFlags.swift` - Feature flags
- `Sources/Core/ZetaModel.swift` - Model loading
- `Sources/Core/ZetaSampler.swift` - Token sampling
- `Sources/Engines/ZetaGGUFEngine.swift` - Main engine

### **Tests**
- `Tests/ZetaQuantizationTests.swift` - Quantization tests
- `Tests/ZetaLongContextTests.swift` - Long-context tests
- `Tests/run_all_tests.sh` - Test runner

### **Benchmarks**
- `Benchmarks/run_all.sh` - Benchmark runner
- `Benchmarks/analyze_results.py` - Results analyzer
- `Benchmarks/RESULTS.md` - Generated results

---

## 🎯 **Quick Checklist**

Before committing:
- [ ] `swift test` passes
- [ ] `./Tests/run_all_tests.sh` passes
- [ ] Code compiles: `swift build -c release`
- [ ] No warnings

Before release:
- [ ] All tests pass
- [ ] Benchmarks run successfully
- [ ] Documentation updated
- [ ] License system tested

---

**Last Updated:** December 9, 2025




















