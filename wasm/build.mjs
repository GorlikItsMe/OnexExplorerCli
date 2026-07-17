import { execSync } from "child_process";
import { copyFileSync, existsSync, mkdirSync, rmSync } from "fs";
import { join, dirname } from "path";
import { fileURLToPath } from "url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const buildDir = join(__dirname, "build");
const wasmOutDir = join(__dirname, "wasm");

function run(cmd, cwd = __dirname) {
  console.log(`> ${cmd}`);
  execSync(cmd, { cwd, stdio: "inherit" });
}

// 1. Configure with emcmake
console.log("\n=== Configuring WASM build ===");
if (!existsSync(buildDir)) {
  mkdirSync(buildDir, { recursive: true });
}
run(`emcmake cmake -B ${buildDir} -S ${__dirname} -DCMAKE_BUILD_TYPE=Release`, __dirname);

// 2. Build
console.log("\n=== Building WASM module ===");
run(`cmake --build ${buildDir} -j$(nproc)`, __dirname);

// 3. Copy WASM output to package directory
console.log("\n=== Copying WASM output ===");
if (!existsSync(wasmOutDir)) {
  mkdirSync(wasmOutDir, { recursive: true });
}

const wasmFiles = ["onex_explorer_wasm.js", "onex_explorer_wasm.wasm"];
const dtsCandidates = ["onex_explorer_wasm.d.ts", "onex.d.ts"];
for (const file of wasmFiles) {
  const src = join(buildDir, file);
  const dst = join(wasmOutDir, file);
  if (existsSync(src)) {
    copyFileSync(src, dst);
    console.log(`  Copied ${file}`);
  } else {
    console.warn(`  WARNING: ${file} not found in ${buildDir}`);
  }
}

// TypeScript declaration - try emscripten-generated, fall back to the shim
let dtsCopied = false;
for (const file of dtsCandidates) {
  const src = join(buildDir, file);
  if (existsSync(src)) {
    copyFileSync(src, join(wasmOutDir, "onex_explorer_wasm.d.ts"));
    console.log(`  Copied ${file} -> onex_explorer_wasm.d.ts`);
    dtsCopied = true;
    break;
  }
}
if (!dtsCopied) {
  console.log("  Using manual type declaration shim for WASM module");
}

// 4. Compile TypeScript
console.log("\n=== Compiling TypeScript ===");
run("npx tsc", __dirname);

console.log("\n=== Build complete ===");
