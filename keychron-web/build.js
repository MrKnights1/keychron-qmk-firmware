import { $ } from "bun";
import { readFileSync, writeFileSync, cpSync } from "fs";

// Bundle JS
await $`bun build src/app.js --outdir dist --format esm --minify`;

// Copy static files
cpSync("src/index.html", "dist/index.html");
cpSync("src/styles", "dist/styles", { recursive: true });

// Cache busting — append timestamp to asset URLs
const ts = Date.now();
let html = readFileSync("dist/index.html", "utf8");
html = html.replace(/src="app\.js"/, `src="app.js?v=${ts}"`);
html = html.replace(/main\.css"/, `main.css?v=${ts}"`);
writeFileSync("dist/index.html", html);

console.log(`Build complete (cache bust: v=${ts})`);
