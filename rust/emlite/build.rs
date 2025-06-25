fn main() {
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();
    if target_os == "emscripten" {
        println!("cargo:rerun-if-changed=ems_env");
        const TOOLCHAIN_SUBPATH: &str = "cmake/Modules/Platform/Emscripten.cmake";
        let emscripten_root = std::path::PathBuf::from(std::env::var("EMSCRIPTEN_ROOT").unwrap());
        let toolchain_file = emscripten_root.join(TOOLCHAIN_SUBPATH);
        let dst = cmk::Config::new("ems_env").define("CMAKE_TOOLCHAIN_FILE", toolchain_file).profile("Release").build();
        println!("cargo:rustc-link-search=naive={}", dst.display());
        println!("cargo:rustc-link-lib=static=ems_env");
    }
}
