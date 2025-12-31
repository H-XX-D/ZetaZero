import { build } from 'esbuild';
import path from 'node:path';
import process from 'node:process';

const rootDir = new URL('..', import.meta.url).pathname;
const entryFile = path.join(rootDir, 'src', 'renderer', 'index.js');
const outFile = path.join(rootDir, 'renderer.bundle.js');

const externalModules = [
  'electron',
  'child_process',
  'fs',
  'path',
  'os',
  'url',
  'util'
];

const commonOptions = {
  entryPoints: [entryFile],
  outfile: outFile,
  bundle: true,
  minify: false,
  sourcemap: true,
  format: 'iife',
  platform: 'neutral',
  target: ['chrome110', 'safari16'],
  external: externalModules,
  define: {
    'process.env.NODE_ENV': JSON.stringify(process.env.NODE_ENV || 'development')
  }
};

const watch = process.argv.includes('--watch');

async function runBuild() {
  if (watch) {
    const ctx = await build({
      ...commonOptions,
      watch: {
        onRebuild(error) {
          if (error) {
            console.error('Renderer rebuild failed:', error);
          } else {
            console.log('Renderer rebuilt:', new Date().toLocaleTimeString());
          }
        }
      }
    });

    console.log('Watching renderer source files...');
    return ctx;
  } else {
    await build(commonOptions);
    console.log('Renderer bundle created at renderer.bundle.js');
  }
}

runBuild().catch((err) => {
  console.error(err);
  process.exit(1);
});

