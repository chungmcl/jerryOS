fn main() {
  println!("cargo:rerun-if-changed=src/entry.S");
  cc::Build::new()
    .file("src/entry.S")
    .compile("entry");
}
