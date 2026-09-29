fn main() {
    cc::Build::new().file("src/foo.c").compile("foo"); // produces libfoo.a, auto-linked
}
