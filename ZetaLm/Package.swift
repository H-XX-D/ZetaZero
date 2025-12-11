// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "ZetaLm",
    platforms: [
        .macOS(.v14),
        .iOS(.v17)
    ],
    products: [
        .library(name: "ZetaCore", targets: ["ZetaCore"]),
        .library(name: "ZetaFeatures", targets: ["ZetaFeatures"]),
        .executable(name: "ZetaCLI", targets: ["ZetaCLI"])
    ],
    targets: [
        // Phase 1: Core inference engine (Metal native)
        .target(
            name: "ZetaCore",
            path: "Sources/ZetaCore",
            resources: [
                .process("Shaders")
            ]
        ),

        // Phase 2: Z.E.T.A. features layer
        .target(
            name: "ZetaFeatures",
            dependencies: ["ZetaCore"],
            path: "Sources/ZetaFeatures"
        ),

        // Phase 3: UI (separate target)
        .target(
            name: "ZetaUI",
            dependencies: ["ZetaCore", "ZetaFeatures"],
            path: "Sources/ZetaUI"
        ),

        // CLI for testing
        .executableTarget(
            name: "ZetaCLI",
            dependencies: ["ZetaCore", "ZetaFeatures"],
            path: "CLI"
        ),

        // Tests
        .testTarget(
            name: "ZetaCoreTests",
            dependencies: ["ZetaCore"],
            path: "Tests"
        )
    ]
)
