import { readFile } from 'node:fs/promises';
import { argv } from 'node:process';

/*
 * This Node.js script reads the contents of a single WebAssembly module
 * and prints details about the module's imports and exports to stdout.
 *
 * The path to the WebAssembly module is expected to be the only additional
 * argument provided to this script.
 *
 * That is, the script is called like this:
 *
 *   node path/to/this/script.mjs path/to/wasm/module.wasm
 */

const numberOfStandardArgs = 2; // The first two values in `argv` are always (path to `node`) and (path to script being executed), see https://nodejs.org/docs/latest-v20.x/api/process.html#processargv
if (argv.length !== numberOfStandardArgs + 1) {
  console.error(`ERROR: Expected just one additional argument, a path to a WebAssembly module on disk, but instead you gave ${argv.length - numberOfStandardArgs} additional args.`);
  process.exit(1);
} else {
  const pathToWasmFile = argv[2];

  const module = await WebAssembly.compile(
    await readFile(pathToWasmFile),
  );

  // Reported module imports have this shape: { module: string; name: string; kind: string; }
  //  see documentation here: https://developer.mozilla.org/en-US/docs/WebAssembly/JavaScript_interface/Module/imports_static#using_imports
  const imports = WebAssembly.Module.imports(module).map((x) => `${x.kind} ${x.module}.${x.name}`);

  // Reported module exports have this shape: { name: string; kind: string; }
  //  see documentation here: https://developer.mozilla.org/en-US/docs/WebAssembly/JavaScript_interface/Module/exports_static#using_exports
  const exports = WebAssembly.Module.exports(module).map((x) => `${x.kind} ${x.name}`);

  console.log('imports:');
  imports.sort().forEach((x) => console.log(`  ${x}`));
  console.log();
  console.log('exports:');
  exports.sort().forEach((x) => console.log(`  ${x}`));
}
