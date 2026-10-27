# List All Tokens

基于 Qt 6 Widgets 的 Windows 桌面应用，用于本地记录不同官方平台或中转站的 token 使用量。

## 功能

- SQLite 本地持久化
- 记录平台、端点、日期、输入 token、输出 token 和备注
- 按平台和日期筛选
- 输入、输出、总 token 汇总
- 删除记录和 UTF-8 CSV 导出

应用只保存用户主动录入的统计数据，不上传 API 密钥或 token。

## 构建

需要 Qt 6（Widgets、Sql）和 CMake 3.21+：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2022_64
cmake --build build --config Release
```

使用 `package.ps1` 可以在 Windows 上构建并收集 Qt 运行库：

```powershell
.\package.ps1 -QtRoot C:\Qt\6.8.3\mingw_64
```

脚本会生成 `dist\list-all-tokens-windows.zip`。后续可以按确定的平台增加本机日志或平台用量 API 适配器。
