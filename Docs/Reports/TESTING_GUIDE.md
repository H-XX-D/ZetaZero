# ZetaLm Testing Guide

**Quick reference for running tests and benchmarks**

---

## 🧪 **Running Tests**

### **All Tests**
```bash
./Tests/run_all_tests.sh
```

### **Specific Test Suites**

#### **Unit Tests**
```bash
swift test
```

#### **Quantization Tests**
```bash
swift test --filter ZetaQuantizationTests
```

#### **Long-Context Tests**
```bash
swift test --filter ZetaLongContextTests
```

#### **Coherence Tests** (requires model)
```bash
./Tests/test_coherence_simple.sh
```

#### **Variational Tests** (requires model)
```bash
./Tests/test_variational_coherence.sh
```

---

## 📊 **Running Benchmarks**

### **Full Benchmark Suite**
```bash
./Benchmarks/run_all.sh [model_path] [steps] [prompt]
```

**Example:**
```bash
./Benchmarks/run_all.sh models/tinyllama-1.1b-chat-v1.0.Q4_0.gguf 100 "Hello"
```

### **Analyze Results**
```bash
python3 Benchmarks/analyze_results.py
```

**Output:** `Benchmarks/RESULTS.md`

---

## 🔍 **Memory Profiling**

### **Profile Memory Usage**
```bash
./Tests/profile_memory.sh [model_path] [steps]
```

**Example:**
```bash
./Tests/profile_memory.sh models/tinyllama-1.1b-chat-v1.0.Q4_0.gguf 100
```

**What to check:**
- Memory leaks (red flags in Instruments)
- Unbounded memory growth
- VRAM usage patterns
- Buffer allocations

---

## ✅ **KV Cache Verification**

### **Verify KV Cache Correctness**
```bash
./Tests/verify_kv_cache.sh [model_path]
```

**What it tests:**
- Position independence
- Long-context stability
- Cache correctness

---

## 📋 **Test Checklist**

Before committing code, run:

- [ ] `swift test` - All unit tests pass
- [ ] `./Tests/run_all_tests.sh` - All integration tests pass
- [ ] `./Benchmarks/run_all.sh` - Benchmarks run successfully
- [ ] `./Tests/profile_memory.sh` - No memory leaks (manual check)

---

## 🐛 **Debugging Tests**

### **Run Single Test**
```bash
swift test --filter ZetaQuantizationTests.testFloat16ToFloatConversion
```

### **Verbose Output**
```bash
swift test --verbose
```

### **Run Tests in Parallel**
```bash
swift test --parallel
```

---

## 📊 **Benchmark Results**

Results are saved to:
- `Benchmarks/results/benchmark_*.csv` - Raw data
- `Benchmarks/RESULTS.md` - Formatted report

---

## 🔧 **Troubleshooting**

### **Tests Fail to Build**
```bash
swift build -c release
swift test
```

### **Benchmark Script Fails**
- Check model path exists
- Verify model format (GGUF)
- Check llama.cpp is installed (optional)

### **Memory Profiling Issues**
- Ensure Instruments.app is installed
- Check macOS version (requires macOS 10.15+)
- Verify Metal device is available

---

## 📝 **Writing New Tests**

### **Unit Test Template**
```swift
import XCTest
@testable import ZetaLm

final class MyTests: XCTestCase {
    func testSomething() {
        // Test code
        XCTAssertEqual(expected, actual)
    }
}
```

### **Integration Test Template**
```bash
#!/bin/bash
# Test script
set -e
swift build -c release
swift run -c release ZetaCLI ...
```

---

**Last Updated:** December 9, 2025




















