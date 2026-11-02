#!/bin/sh
# Builds web/belgian-sauna.html: the same engine compiled to WebAssembly, for phones and browsers.
# Needs wasi-sdk (github.com/WebAssembly/wasi-sdk) at $WASI_SDK, plus g++, python3.
set -e
cd "$(dirname "$0")"
$WASI_SDK/bin/clang++ --target=wasm32-wasip1 --sysroot=$WASI_SDK/share/wasi-sysroot -O3 -std=c++17 -fno-exceptions -D_USE_MATH_DEFINES \
  -I../Source -mexec-model=reactor -Wl,--strip-all -Wl,-z,stack-size=1048576 -Wl,--initial-memory=33554432 -Wl,--max-memory=268435456 \
  -o sauna.wasm wasm_api.cpp ../Source/airwindows/Unit_*.cpp -w
g++ -std=c++17 -I../Source -o gen_data gen_data.cpp && ./gen_data > sauna_data.json && rm gen_data
python3 - <<'PY'
import base64, pathlib
p = pathlib.Path('page.html').read_text()
p = p.replace('/*DATA*/', pathlib.Path('sauna_data.json').read_text().strip())
p = p.replace('/*WASM*/', base64.b64encode(pathlib.Path('sauna.wasm').read_bytes()).decode())
p = p.replace('/*UI*/', pathlib.Path('ui.js').read_text())
pathlib.Path('belgian-sauna.html').write_text(p)
PY
echo "Built web/belgian-sauna.html"
