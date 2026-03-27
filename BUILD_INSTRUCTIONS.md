# Build Instructions for PvZ (OOPLab)

## 1. 目錄切換

```bash
cd /Users/xunxun/OOPLab
```

## 2. 刪除先前 build 目錄（保證乾淨）

```bash
rm -rf build
```

## 3. CMake 配置（必須 Debug）

此專案目前只支持 `Debug` 模式，`Release` 會出現錯誤。B

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -B build
```

如果你想 suppress dev warning（可選）：

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -Wno-dev -B build
```

## 4. 編譯

```bash
cmake --build build
```

## 5. 執行

執行生成的目標程式，通常是 `PvZ`：

```bash
./build/PvZ
```

如果檔名不同，可先檢查：

```bash
ls build
```

## 6. 常見錯誤與解決

- `relative RESOURCE_DIR is WIP` / `relative PTSD_ASSETS_DIR is WIP`
  - 必須使用 `-DCMAKE_BUILD_TYPE=Debug`

- `no member named 'SetTranslation' in 'Util::Renderer'`
  - 源碼已修正：`m_Root.SetTranslation(...)` 改為 `m_Map->m_Transform.translation = ...`

- `zsh: no such file or directory: ./build/PvZ`
  - 可能是編譯失敗，先解決 CMake 或編譯錯誤再執行

## 7. 參考

- `README.md`（已有簡易快速啟動說明）
- `CMakeLists.txt`（執行路徑與 flags 定義）
