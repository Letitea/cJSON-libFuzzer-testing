# cJSON-libFuzzer-testing

本專案使用 **LLVM libFuzzer** 與 **AddressSanitizer (ASan)** 對 `cJSON` 進行 in-process、覆蓋率導向的模糊測試（Coverage-Guided Fuzz Testing），重點在於發掘畸形輸入、邊界條件、記憶體配置失敗與指標操作所引發的記憶體安全問題（如 Out-of-Bounds Read/Write、Use-After-Free、Double-Free 與 Memory Leak）。

---

## Fuzz Targets 一覽

專案於 [CMakeLists.txt](CMakeLists.txt) 定義了 7 個 Fuzz Targets，皆以 `-O1 -g -fno-omit-frame-pointer -fsanitize=fuzzer,address` 編譯：

| Target | 原始碼 | 測試重點與涵蓋的 API | 對應種子目錄 |
| :--- | :--- | :--- | :--- |
| `fuzz_parse` | [fuzz/fuzz_parse.c](fuzz/fuzz_parse.c) | `cJSON_Parse()`、`cJSON_Delete()`（解析畸形、深層巢狀、Unicode 與數值邊界） | `corpus/parse/` |
| `fuzz_print` | [fuzz/fuzz_print.c](fuzz/fuzz_print.c) | `cJSON_Parse()`、`cJSON_Print()`、`cJSON_PrintUnformatted()` | `corpus/print/` |
| `fuzz_minify` | [fuzz/fuzz_minify.c](fuzz/fuzz_minify.c) | `cJSON_Minify()`（空白、換行、Tab 與 C/C++ 註解處理） | `corpus/minify/` |
| `fuzz_boundary` | [fuzz/fuzz_boundary.c](fuzz/fuzz_boundary.c) | NULL 指標、空字串與越界索引（`cJSON_GetArrayItem`）防禦測試 | `corpus/boundary/` |
| `fuzz_reference` | [fuzz/fuzz_reference.c](fuzz/fuzz_reference.c) | `cJSON_AddItemReferenceToArray()` 與參考節點釋放（UAF / Double-Free） | `corpus/reference/` |
| `fuzz_memory_structure` | [fuzz/fuzz_memory_structure.c](fuzz/fuzz_memory_structure.c) | `cJSON_Duplicate()`、`cJSON_AddItemToObject()`、`cJSON_DetachItemFromObject()` 與雙向鏈結串列一致性驗證 | `corpus/memory_structure/` |
| `fuzz_hooks` | [fuzz/fuzz_hooks.c](fuzz/fuzz_hooks.c) | 透過 `cJSON_InitHooks()` 模擬 OOM（記憶體配置失敗）下的解析、列印與釋放 | `corpus/hooks/` |

---

## 工具與環境

- **Docker**（`ubuntu:24.04` 映像檔，定義於 [Dockerfile](Dockerfile)）
- **Clang / LLVM libFuzzer**
- **AddressSanitizer (ASan)**（含 LeakSanitizer）
- **CMake (>= 3.20)**
- **GitHub Actions CI**（[.github/workflows/fuzz-smoke.yml](.github/workflows/fuzz-smoke.yml)）

---

## 專案目錄結構

```text
.
├── .github/workflows/   # GitHub Actions CI（fuzz-smoke.yml）
├── artifacts/           # 本機執行產生的 crash / leak / 暫存輸出（已設 .gitignore）
├── build/               # CMake 編譯輸出目錄（已設 .gitignore）
├── corpus/              # 各 Fuzz Target 的種子測資（含人工設計與分類種子）
│   ├── boundary/
│   ├── hooks/
│   ├── memory_structure/
│   ├── minify/
│   ├── parse/
│   ├── print/
│   └── reference/
├── fuzz/                # libFuzzer harness 原始碼
├── reports/             # 實驗記錄、分析輸出與期末報告
├── scripts/             # 輔助腳本
├── tests/               # 單元 / 回歸測試
├── third_party/cJSON/   # 受測目標 cJSON 原始碼
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

進入容器後使用 Clang 編譯所有 Fuzz Targets：

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
```

#### 避免污染乾淨 `corpus/` 的進階執行方式
建議將新產生的突變語料與 crash 產物導向 `artifacts/`，維持 `corpus/` 的整潔：

```bash
mkdir -p artifacts/out_parse
./build/fuzz_parse -max_total_time=300 -artifact_prefix=artifacts/ -print_final_stats=1 artifacts/out_parse corpus/parse
```

#### 複現單一 Crash / Leak 檔案
當發現 `artifacts/crash-*` 或 `artifacts/leak-*` 時，可直接將檔案傳入對應的執行檔重現 ASan 堆疊追蹤：

```bash
./build/fuzz_parse artifacts/crash-xxxx
```

---

## CI 自動化測試

[.github/workflows/fuzz-smoke.yml](.github/workflows/fuzz-smoke.yml) 會在以下時機觸發 Smoke Test（建置 Docker 映像、編譯並執行核心 fuzzer 各 10 秒）：
- Push 至 `main` 分支
- 發起或更新目標為 `main` 的 Pull Request
- 於 GitHub Actions 頁面手動觸發（`workflow_dispatch`）

---


## 參考資料

- [cJSON Repository](https://github.com/DaveGamble/cJSON)
- [LLVM libFuzzer Documentation](https://llvm.org/docs/LibFuzzer.html)
- [Clang AddressSanitizer Documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
