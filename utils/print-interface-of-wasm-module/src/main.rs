/*
 * This Rust app reads the contents of a single WebAssembly module
 * and prints details about the module's imports and exports to stdout.
 *
 * The path to the WebAssembly module is expected to be the only additional
 * command-line argument provided to this app.
 */

use std::env;
use std::process;
use itertools::Itertools;

use wasmtime::*;

struct Config {
    path_to_wasm_module: String,
}

impl Config {
    pub fn new(args: &[String]) -> Result<Config, &'static str> {
        if args.len() != 2 {
            return Err("Expected just one additional argument, a path to a WebAssembly module on disk.")
        }
        let path_to_wasm_module = args[1].clone();

        Ok(Config { path_to_wasm_module })
    }
}

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();

    // Parse command-line arguments
    let config = Config::new(&args).unwrap_or_else(|err| {
        eprintln!("Problem parsing arguments: {}", err);
        process::exit(1);
    });

    // Load WebAssembly module
    let engine = Engine::default();
    let module = Module::from_file(&engine, config.path_to_wasm_module).unwrap_or_else(|err| {
        eprintln!("Problem loading WebAssembly module: {}", err);
        process::exit(1);
    });

    fn type_name(extern_type: &ExternType) -> &'static str {
        use ExternType::*;
        match extern_type {
            Func(_) => "function",
            Global(_) => "global",
            Table(_) => "table",
            Memory(_) => "memory",
        }
    }

    let imports = module.imports().map(
        |x| format!("{ty} {module}.{name}", ty = type_name(&x.ty()), module = x.module(), name = x.name())
    );
    let exports = module.exports().map(
        |x| format!("{ty} {name}", ty = type_name(&x.ty()), name = x.name())
    );

    println!("imports:");
    for import in imports.sorted() {
        println!("  {import}");
    }
    println!("");
    println!("exports:");
    for export in exports.sorted() {
        println!("  {export}");
    }

    Ok(())
}
