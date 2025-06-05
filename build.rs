use std::fs;
use std::path::PathBuf;
use cc::Build;

fn build_c_lib(mut bob_the_builder: Build, lib_name: &str, dep_folder_names: &[&'static str]) {
  let lib_dir = format!("src/{}", lib_name);
  let lib_include_dir = format!("{}/include", lib_dir);
  bob_the_builder
    .define("__RUST__", None)
    .flag("-nostdlib")
    .flag("-nostdinc")
    .flag("-fno-builtin")
    .flag("-static")
    .flag("-fno-unwind-tables")
    .flag("-fno-asynchronous-unwind-tables")
    .include("src/include")
  ;

  let mut bindings = bindgen::Builder::default()
    .use_core()
    .ctypes_prefix("core::ffi") 
    .header(format!("{}/{}.h", lib_include_dir, lib_name))
    .clang_arg("-nostdlib")
    .clang_arg("-nostdinc")
    .clang_arg("-fno-builtin")
    .clang_arg("-static")
    .clang_arg("-Isrc/include") // Include top level src/include/jerryOS.h
    // .generate_comments(false)
  ;

  fn add_unit(dir: &str, builder: &mut Build, binder: bindgen::Builder) -> bindgen::Builder {
    println!("cargo:rerun-if-changed=src/{}", dir);
    let include_dir = format!("src/{}/include", dir);
    builder.include(&include_dir);
    for entry in fs::read_dir(format!("src/{}/", dir)).unwrap() {
      let path = entry.unwrap().path();
      if path.extension().and_then(|s| s.to_str()) == Some("c") {
        builder.file(path);
      }
    }
    binder.clang_arg(format!("-I{}", &include_dir))
  }

  for dep_folder_name in dep_folder_names {
    bindings = add_unit(dep_folder_name, &mut bob_the_builder, bindings);
  }
  bindings = add_unit(lib_name, &mut bob_the_builder, bindings);
  
  let out_path = PathBuf::from("src/CBindings");
  bindings
    .generate()
    .expect("Bindings failed to generate!")
    .write_to_file(out_path.join(format!("{}.rs", lib_name)))
    .expect("Bindings failed to be written!");

  bob_the_builder
    .compile(&lib_name)
  ;
} 

fn main() {
  println!("cargo:rerun-if-changed=src/entry.S");
  println!("cargo:rerun-if-changed=path/to/link.lds");

  Build::new()
    .file("src/entry.S")
    .compile("entry")
  ;

  build_c_lib(
    Build::new(), 
    "libfdtLite", 
    &["jerryLibc"]
  );
}
