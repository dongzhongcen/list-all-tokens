# List All Tokens

基于 Qt 6 Widgets 的 Windows 桌面应用，用于本地记录不同官方平台或中转站的 token 使用量。

## 功能

- SQLite 本地持久化
- 记录平台、端点、日期、输入 token、输出 token 和备注
- 按平台和日期筛选
- 输入、输出、总 token 汇总
- 删除记录和 UTF-8 CSV 导出
- 手动填写中转站用量接口地址和 API Token，点击按钮自动请求并记录用量
- 自动识别常见 JSON 字段：`input_tokens`、`prompt_tokens`、`output_tokens`、`completion_tokens`、`total_tokens`

应用只在请求时使用内存中的 API Token，不写入数据库；接口地址和返回数据不会上传到第三方。

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
