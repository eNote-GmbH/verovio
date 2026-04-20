// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "Verovio",
    platforms: [
        .iOS(.v15),
        .macCatalyst(.v15)
    ],
    products: [
        .library(name: "Verovio", targets: ["Verovio"])
    ],
    targets: [
        .target(
            name: "Verovio",
            path: ".",
            exclude: [
                "Verovio.podspec",
                "Verovio.xcodeproj",
                "appveyor.yml",
                "bindings",
                "build_xcframework.sh",
                "cmake",
                "CHANGELOG.md",
                "codemeta.json",
                "COPYING",
                "COPYING.LESSER",
                "data",
                "doc",
                "Dockerfile",
                "emscripten",
                "fonts",
                "gitlab",
                "libmei/AUTHORS",
                "libmei/README.md",
                "libmei/config.yml",
                "libmei/datatypes.yml",
                "libmei/mei",
                "libmei/poetry.lock",
                "libmei/pyproject.toml",
                "libmei/tools",
                "MANIFEST.in",
                "README.md",
                "scripts",
                "setup.py",
                "tools/get_git_commit.sh",
                "tools/main.cpp"
            ],
            sources: [
                "src",
                "libmei/dist",
                "libmei/addons",
                "tools/c_wrapper.cpp"
            ],
            publicHeadersPath: "include/Verovio",
            cxxSettings: [
                .headerSearchPath("src"),
                .headerSearchPath("include/crc"),
                .headerSearchPath("include/hum"),
                .headerSearchPath("include/json"),
                .headerSearchPath("include/midi"),
                .headerSearchPath("include/pugi"),
                .headerSearchPath("include/vrv"),
                .headerSearchPath("include/zip"),
                .headerSearchPath("libmei/dist"),
                .headerSearchPath("libmei/addons"),
                .define("NO_HUMDRUM_SUPPORT"),
                .define("NO_ABC_SUPPORT"),
                .define("NO_PAE_SUPPORT")
            ]
        )
    ],
    cxxLanguageStandard: .cxx17
)
