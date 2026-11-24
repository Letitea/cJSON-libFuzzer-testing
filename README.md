# cJSON-libFuzzer-testing

本專案使用 **LLVM libFuzzer** 與 **AddressSanitizer (ASan)** 對 `cJSON` (v1.7.19) 進行 in-process、覆蓋率導向的模糊測試（Coverage-Guided Fuzz Testing），重點在於發掘畸形輸入、邊界條件、記憶體配置失敗（OOM）與指標操作所引發的記憶體安全問題（如 Out-of-Bounds Read/Write、Use-After-Free、Double-Free 與 Memory Leak）。

透過 10 個解耦的 Fuzz Targets，專案已涵蓋 `cJSON.c` **全部 78 個公開 API 與 35 個內部函式（Function Coverage 100.00%、Line Coverage 95.94%、Branch Coverage 91.65%）**，並在 OOM 錯誤注入測試中發掘了 `cJSON` v1.7.19 上游程式碼於 `cJSON_AddItemReferenceToObject()` 的真實記憶體洩漏缺陷。

- **線上覆蓋率儀表板（GitHub Pages）**：https://letitea.github.io/cJSON-libFuzzer-testing/

---

## Fuzz Targets 一覽

專案於 [CMakeLists.txt](CMakeLists.txt) 定義了 **10 個 Fuzz Targets**，皆以 `-O1 -g -fno-omit-frame-pointer -fsanitize=fuzzer,address` 編譯：

| Target | 原始碼 | 測試重點與涵蓋的 API | 對應種子目錄 |
| :--- | :--- | :--- | :--- |
| `fuzz_parse` | [fuzz/fuzz_parse.c](fuzz/fuzz_parse.c) | `cJSON_Parse()`、`cJSON_ParseWithLength()`、`cJSON_ParseWithOpts()`、`cJSON_ParseWithLengthOpts()`、`cJSON_GetErrorPtr()`、`cJSON_Version()`、`cJSON_malloc()`／`cJSON_free()` | `corpus/parse/` |
| `fuzz_print` | [fuzz/fuzz_print.c](fuzz/fuzz_print.c) | `cJSON_Print()`、`cJSON_PrintUnformatted()`、`cJSON_PrintBuffered()`、`cJSON_PrintPreallocated()`、`cJSON_Raw`／`cJSON_Invalid` 列印與 `CJSON_NESTING_LIMIT` 深度保護 | `corpus/print/` |
| `fuzz_minify` | [fuzz/fuzz_minify.c](fuzz/fuzz_minify.c) | `cJSON_Minify()`（空白、換行、Tab 與 C/C++ 註解原地壓縮處理） | `corpus/minify/` |
| `fuzz_boundary` | [fuzz/fuzz_boundary.c](fuzz/fuzz_boundary.c) | 各 API 之 `NULL` 指標、空字串、負數／越界索引、損毀節點、`case_insensitive_strcmp` 邊界與 `CJSON_CIRCULAR_LIMIT` 循環參照防禦 | `corpus/boundary/` |
| `fuzz_reference` | [fuzz/fuzz_reference.c](fuzz/fuzz_reference.c) | `cJSON_AddItemReferenceToArray()`、`cJSON_AddItemReferenceToObject()`、`cJSON_CreateStringReference()`、`cJSON_CreateObjectReference()`、`cJSON_CreateArrayReference()`、`cJSON_AddItemToObjectCS()` | `corpus/reference/` |
| `fuzz_memory_structure` | [fuzz/fuzz_memory_structure.c](fuzz/fuzz_memory_structure.c) | `cJSON_Duplicate()`（遞迴與非遞迴）、`cJSON_AddItemToObject()`、`cJSON_DetachItemFromObject()` 與雙向鏈結串列一致性驗證（`child->prev == tail`） | `corpus/memory_structure/` |
| `fuzz_hooks` | [fuzz/fuzz_hooks.c](fuzz/fuzz_hooks.c) | 透過 `cJSON_InitHooks()` 動態注入 OOM（`malloc` 失敗），驗證 Parse、Print、PrintBuffered、Duplicate、Create\*、Add\*ToObject、Replace 與 Reference 路徑之錯誤恢復與記憶體釋放 | `corpus/hooks/` |
| `fuzz_create_and_types` | [fuzz/fuzz_create_and_types.c](fuzz/fuzz_create_and_types.c) | 全部 `cJSON_Create*`（含 `Int`/`Float`/`Double`/`StringArray`）、9 個 `cJSON_Add*ToObject` 快捷函式、10 個 `cJSON_Is*` 型別檢查、`cJSON_GetStringValue()`、`cJSON_GetNumberValue()` 與相關巨集 | `corpus/create_and_types/` |
| `fuzz_mutate` | [fuzz/fuzz_mutate.c](fuzz/fuzz_mutate.c) | `cJSON_GetArraySize()`、`cJSON_GetObjectItemCaseSensitive()`、`cJSON_HasObjectItem()`、陣列與物件之新增／插入／取代／拆離／刪除（含 CaseSensitive 變體）、`cJSON_SetNumberHelper()`、`cJSON_SetValuestring()` | `corpus/mutate/` |
| `fuzz_compare` | [fuzz/fuzz_compare.c](fuzz/fuzz_compare.c) | `cJSON_Compare()`（大小寫敏感／不敏感、巢狀物件與陣列、不同型別、`cJSON_Raw` 與 `NULL` 邊界比對） | `corpus/compare/` |

---

## 工具與環境

- **Docker**（`ubuntu:24.04` 映像檔，定義於 [Dockerfile](Dockerfile)）
- **Clang / LLVM 18 libFuzzer**
- **AddressSanitizer (ASan)**（含 LeakSanitizer）
- **LLVM Source-Based Code Coverage**（`llvm-profdata` / `llvm-cov`）
- **CMake (>= 3.20)**
- **GitHub Actions CI**（[.github/workflows/fuzz-smoke.yml](.github/workflows/fuzz-smoke.yml)）

---

## 專案目錄結構

```text
.
├── .github/workflows/   # GitHub Actions CI（fuzz-smoke.yml）
├── .vscode/             # VS Code GDB 除錯設定（launch.json、tasks.json）
├── artifacts/           # 本機執行產生的 crash / leak / 覆蓋率報告（已設 .gitignore）
├── build/               # CMake ASan+libFuzzer 編譯輸出（已設 .gitignore）
├── build-cov/           # CMake Coverage 插樁編譯輸出
├── corpus/              # 各 Fuzz Target 的種子測資（10 組分類語料）
│   ├── boundary/
│   ├── compare/
│   ├── create_and_types/
│   ├── hooks/
│   ├── memory_structure/
│   ├── minify/
│   ├── mutate/
│   ├── parse/
│   ├── print/
│   └── reference/
├── fuzz/                # 10 個 libFuzzer harness 原始碼
├── reports/             # 實驗記錄、分析輸出與期末報告
├── scripts/             # 覆蓋率自動化腳本（run_coverage.sh）
├── tests/               # 單元 / 回歸測試
├── third_party/cJSON/   # 受測目標 cJSON 原始碼（v1.7.19）
├── CMakeLists.txt
└── Dockerfile
```

---

## 建置與執行

### 1. 啟動 Docker 環境並編譯

在專案根目錄執行：

```bash
# 建立映像檔（首次執行或 Dockerfile 變更時）
docker build -t cjson-libfuzzer-testing .

# 進入容器並掛載專案目錄（PowerShell 可將 $(pwd) 改為 ${PWD}）
docker run --rm -it -v "$(pwd):/work" -w /work cjson-libfuzzer-testing
```

進入容器後使用 Clang 編譯全部 10 個 Fuzz Targets：

```bash
cmake -S . -B build -DCMAKE_C_COMPILER=clang
cmake --build build --parallel
```

### 2. 執行模糊測試

基本執行方式（以各跑 60 秒為例）：

```bash
./build/fuzz_parse -max_total_time=60 corpus/parse
./build/fuzz_print -max_total_time=60 corpus/print
./build/fuzz_minify -max_total_time=60 corpus/minify
./build/fuzz_boundary -max_total_time=60 corpus/boundary
./build/fuzz_reference -max_total_time=60 corpus/reference
./build/fuzz_memory_structure -max_total_time=60 corpus/memory_structure
./build/fuzz_hooks -max_total_time=60 corpus/hooks
./build/fuzz_create_and_types -max_total_time=60 corpus/create_and_types
./build/fuzz_mutate -max_total_time=60 corpus/mutate
./build/fuzz_compare -max_total_time=60 corpus/compare
```

#### 避免污染乾淨 `corpus/` 的進階執行方式
建議將新產生的突變語料與 crash/leak 產物導向 `artifacts/`，維持 `corpus/` 的整潔：

```bash
mkdir -p artifacts/out_parse
./build/fuzz_parse -max_total_time=300 -artifact_prefix=artifacts/ -print_final_stats=1 artifacts/out_parse corpus/parse
```

#### 複現單一 Crash / Leak 檔案與除錯
當發現 `artifacts/crash-*` 或 `artifacts/leak-*` 時，可直接將檔案傳入對應的執行檔重現 ASan / LSan 堆疊追蹤：

```bash
./build/fuzz_hooks artifacts/leak-xxxx
```

若要使用 VS Code 或 GDB 單步追蹤該測資（避免 GDB 與 LeakSanitizer 的 `ptrace` 衝突，需帶 `ASAN_OPTIONS=detect_leaks=0`）：

```bash
ASAN_OPTIONS=detect_leaks=0 gdb --args ./build/fuzz_hooks artifacts/leak-xxxx
```

---

## 覆蓋率報告（llvm-cov 自動化腳本）

專案提供 [scripts/run_coverage.sh](scripts/run_coverage.sh)，可一鍵以 `-fprofile-instr-generate -fcoverage-mapping` 編譯 `build-cov/`、重播全部 10 組語料、合併 `.profraw`，並產出終端機摘要與 HTML 視覺化報告：

```bash
bash scripts/run_coverage.sh
```

完成後於本機開啟 `artifacts/cov/html/index.html`，或直接檢視由 CI/CD 自動部署的線上覆蓋率報告：https://letitea.github.io/cJSON-libFuzzer-testing/

### 最新覆蓋率實測結果（`third_party/cJSON/cJSON.c`）

| 指標 | 初版（7 Targets） | 擴充後（10 Targets） | 提升幅度 |
| :--- | :---: | :---: | :---: |
| **Function Coverage** | 45.13% (51 / 113) | **100.00% (113 / 113)** | +54.87% |
| **Line Coverage** | 55.83% (1,279 / 2,291) | **95.94% (2,198 / 2,291)** | +40.11% |
| **Region Coverage** | 59.38% (975 / 1,642) | **97.32% (1,598 / 1,642)** | +37.94% |
| **Branch Coverage** | 55.31% (583 / 1,054) | **91.65% (966 / 1,054)** | +36.34% |

---

## 已知上游漏洞發現（cJSON v1.7.19）

透過 `fuzz_hooks` 進行 OOM 錯誤注入時，於 `third_party/cJSON/cJSON.c:2147` 發現 `cJSON_AddItemReferenceToObject()` 的真實記憶體洩漏（CWE-401）：

- **成因**：函式先呼叫 `create_reference()` 配置 64 bytes 參考節點，並直接傳入 `add_item_to_object()`；當 `add_item_to_object()` 內的 `cJSON_strdup()` 因 OOM 失敗回傳 `false` 時，該參考節點未被掛載也未呼叫 `cJSON_Delete()` 釋放，導致呼叫端無法回收而產生 64 bytes Direct Leak。
- **複現方式**：執行 `./build/fuzz_hooks -max_total_time=10 -artifact_prefix=artifacts/ corpus/hooks`（若需跑完整覆蓋率腳本或 CI Smoke Test，可先將 `fuzz/fuzz_hooks.c` 中該行呼叫註解或於重播時加上 `-detect_leaks=0`）。

---

## CI/CD 自動化流水線（GitHub Actions + GitHub Pages）

[.github/workflows/fuzz-smoke.yml](.github/workflows/fuzz-smoke.yml) 實作完整的 **CI（持續整合）＋ CD（持續交付）** 自動化流程，於 Push 至 `master`、發起 Pull Request 或手動觸發（`workflow_dispatch`）時執行：

1. **CI 階段（`fuzz_smoke`）**：遞迴檢出 submodules、建置 Docker 映像、編譯並對全部 10 個 Fuzz Target 各執行 10 秒 ASan 冒煙測試（失敗時自動上傳 Crash Artifacts），並執行 [scripts/run_coverage.sh](scripts/run_coverage.sh) 產出最新 `llvm-cov` 覆蓋率摘要（輸出至 Actions Step Summary）與 HTML 報告。
2. **CD 階段（`deploy_coverage_pages`）**：當 `master` 分支 CI 通過後，自動透過 `actions/upload-pages-artifact` 與 `actions/deploy-pages` 將最新 `llvm-cov` HTML 視覺化覆蓋率儀表板持續交付部署至 **GitHub Pages**：https://letitea.github.io/cJSON-libFuzzer-testing/（首次使用需先於 GitHub Repository 的 **Settings → Pages → Build and deployment → Source** 選擇 **GitHub Actions**）。

---

## 參考資料

- [cJSON Repository](https://github.com/DaveGamble/cJSON)
- [LLVM libFuzzer Documentation](https://llvm.org/docs/LibFuzzer.html)
- [Clang AddressSanitizer Documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
