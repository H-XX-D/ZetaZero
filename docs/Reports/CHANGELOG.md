# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
-   Initial release of ZetaLm.
-   `ZetaGGUFEngine`: Core inference engine for GGUF models.
-   `ZetaGGUFKernels`: Metal kernels for Q4_0 matrix multiplication, RMSNorm, RoPE, and Zeta Attention.
-   `QuantumLayers`: Reference implementation for quantum-inspired layers.
-   `zeta_surgery.py`: Python script for model weight adaptation.
-   Support for temporal attention decay mechanism (Time-decay + Gating).
-   Integration with ThirdLM CLI.

### Changed
-   Refactored project structure to isolate Zeta components into `ZetaLm/`.

### Fixed
-   N/A

## [0.1.0] - 2025-12-08
### Added
-   Proof of concept implementation.
