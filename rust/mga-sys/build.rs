fn main() {
    println!("cargo:rerun-if-changed=../../mg_arena.h");
    println!("cargo:rerun-if-changed=mg_arena_impl.c");

    cc::Build::new().file("mg_arena_impl.c").compile("mg_arena");
}
