# Phigros Chart Merge

Phigros 谱面文件合并工具，专为解决传统合并工具判定线冗余问题设计，更智能地拼接谱面片段。

## 功能介绍

与传统工具直接拼接判定线的方式不同，本工具在设计上的用途为：

- 支持拼接同一张谱面的不同片段
- 适配多人合作写谱场景，可拼接制作的谱面片段

本工具支持：

- 单独调整特定判定线的时间范围和复制内容（音符、事件等）
- 合并后谱面自动继承最顶端谱面的元数据、判定线数量等核心信息

## 使用方法

1. **导入文件**：拖拽 JSON 或 PEZ 格式文件至卡片拖拽区域，或点击该区域手动上传本地文件；
2. **基础配置**：配置谱面拼接片段的开始时间与结束时间，根据需求勾选是否复制事件与音符；
3. **高级设置**：点击“添加”按钮，可新增谱面槽位以导入更多片段；
4. **管理片段**：通过“上移”“下移”按钮，调整谱面片段的合并顺序；
5. **执行合并**：完成所有设置后，点击右下角“开始拼接”按钮，选择拼接策略并确认，即可生成合并后的谱面文件。

## 注意事项

- 若载入 PEZ 文件，下载的 JSON 文件名可能与 PEZ 内不一致，需手动修改替换；
- 加载文件后若出现可操作但无响应，且控制台提示内存溢出，建议刷新页面重新载入；
- 工具不强制校验谱面元数据，建议自行确认所有谱面来自同一曲目；
- 如遇无法解决的异常，可尝试刷新页面重新操作。

## 构建方法

### 前置依赖

- CMake 3.13 或更高版本
- Emscripten SDK（用于 WebAssembly 构建）
- Node.js（建议 v22.0.0 或兼容版本）
- 支持 C++17 标准的现代 C++ 编译器
- MinGW-w64 工具链（可选，根据目标平台调整）

### 构建步骤

1. 克隆仓库到本地

```bash
git clone https://github.com/seacblue/phigros-chart-merge.git
cd phigros-chart-merge
```

2. 配置 Emscripten 环境

```bash
source /path/to/emsdk/emsdk_env.sh
```

3. 构建项目

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
cmake --build .
```

或

```bash
cmake --preset wasm-release
cmake --build --preset wasm-build
```

## 技术说明

- **前端界面**：基于 HTML + CSS 实现，包含交互逻辑与用户界面
- **核心逻辑**：使用 C++ 编写，通过 Emscripten 编译为 WebAssembly 供前端调用
- **构建系统**：采用 CMake 进行跨平台构建配置，支持 WebAssembly 目标平台
- **数据存储**：使用 `chart_storage.js` 管理谱面数据（WebAssembly 版本）
