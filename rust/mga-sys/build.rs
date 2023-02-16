fn main() {
    println!("cargo:rerun-if-changed=../../mg_arena.h");
    println!("cargo:rerun-if-changed=mg_arena_impl.c");

    let mut build = cc::Build::new();

    // #define MGA_FORCE_MALLOC
    #[cfg(feature = "malloc")]
    build.define("MGA_FORCE_MALLOC", None);

    build.file("mg_arena_impl.c").compile("mg_arena");
}
