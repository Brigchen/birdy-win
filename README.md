# Birdy Qt

与 `birdy-skill` 平行的 Windows 桌面程序，用 C++ / Qt 重写。鸟检和认种不走 PyTorch，沿用 `birdy/app` 里已经验证过的 ncnn 路径。

界面按 `birdy-skill` 的 Birdy 主窗口排列：顶栏、左右分栏、可收起卡片，覆盖文件夹、地理位置、连拍、物种识别、水印、轨迹图和观鸟记录。设置写入本目录的 `gui_config.json`；第一次启动若还没有这份文件，会读取 `birdy-skill/src/gui_config.json`。

已接上的能力：

- 名录：读取模型目录中的 `bird_info.json`（中文名、英文名、学名三元组）。
- 推理：链接 ncnn 后加载 `bird-seg.param/.bin` 与 `bird-iden.param/.bin`。检测为 640 letterbox，鸟类取 COCO 第 14 类；认种为短边 256、中心裁 224，ImageNet 归一化，softmax 温度 1。

连拍筛选、写入 GPS、物种裁切、水印、拼图、动图、轨迹图和记录导出的处理还在从 Python 迁入。对应按钮会在右侧日志里说明。

## 编译

本机 Anaconda 里已有 Qt 5.15.2（`C:\Users\brigc\anaconda3\Library`）。还需要 CMake 和 MSVC（Visual Studio 2022 的“使用 C++ 的桌面开发”）。

```bat
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Users/brigc/anaconda3/Library
cmake --build build --config Release
```

编完后会把 Qt 运行库拷到 `build/Release`，和 `birdy.exe` 放在一起，这样双击就能打开，不必把 Anaconda 加进 PATH。

ncnn 的 Windows 构建目录通过 CMake 的 `ncnn_DIR` 传入。找不到 ncnn 时程序仍能打开，状态栏会写明推理未链接。

```bat
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Users/brigc/anaconda3/Library -Dncnn_DIR=C:/path/to/ncnn/lib/cmake/ncnn
```

模型文件放在同一个目录：`bird-seg.param`、`bird-seg.bin`、`bird-iden.param`、`bird-iden.bin`、`bird_info.json`。
