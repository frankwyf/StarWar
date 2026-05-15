# StarWar（简体中文）

一个使用现代 C++ 与 OpenGL/GLFW 开发的小型开源 2D 太空射击游戏。

## 当前可玩内容

- 鼠标瞄准 + 推进控制飞船
- 开始前可选择难度（`简单 / 普通 / 困难`）
- 开局有倒计时，不会直接进入战斗
- 对抗陨石（含分裂陨石）
- 对抗会追踪并射击玩家的敌机
- 每 4 波出现 Boss
- 波次推进与武器升级
- 生命值 + 护盾 + 无敌时间 + 走位生存

## 游戏规则与玩法

### 目标

尽可能生存更久，清理波次并击败 Boss，获得更高分数。

### 核心循环

1. 在帮助页按 `Enter` 开始
2. 躲避威胁并射击陨石/敌机
3. 清空敌机后进入下一波
4. 在 `4, 8, 12, ...` 波迎战 Boss
5. 重复循环，挑战更高强度

### 操作

- **1 / 2 / 3**：在开始页选择难度（简单/普通/困难）
- **Enter**：从帮助页开始（进入倒计时）
- **鼠标**：瞄准
- **鼠标左键**：开火
- **W / ↑**：推进
- **鼠标右键**：推进（可替代）
- **Space**：切换驾驶模式
- **R**：重开
- **Esc**：退出

### 战斗与成长

- 击毁陨石可得分
- 大陨石被击中后可能分裂为小陨石
- 敌机会朝玩家方向开火
- Boss 血量更高，且会多方向弹幕
- 武器会随波次提升：
  - **Lv1**：单发
  - **Lv2**：三连散射
  - **Lv3**：更快射速 + 更高伤害

### 生存建议

- 不要原地停留，持续移动
- 用短促推进规避弹幕
- 普通波次尽快清敌，降低 Boss 波压力
- 受击后无敌期用于重新站位

## HUD 信息

- 窗口标题显示：**Score / Lives / Wave / Weapon / Difficulty**
- 左上：分数进度条
- 左上第二条：护盾条
- 右上：生命指示
- Boss 头顶：血条

## 玩家直接游玩（推荐）

1. 打开 GitHub 的 **Releases** 页面
2. 下载 `StarWar-windows-x64.zip`
3. 解压后运行 `StarWar.exe`

## 从源码构建

### Windows（Visual Studio）

1. 打开 `StarWar.sln`
2. 选择 `x64` 与 `Debug` 或 `Release`
3. 构建并运行 `main` 项目

### 跨平台（Premake + Make）

```bash
premake5 gmake2
make config=release_x64
```

## 自动构建与发布

- 工作流文件：`.github/workflows/release-windows.yml`
- 每次推送到 `main/master` 都会生成可下载构建产物
- 打 `v1.0.0` 这类标签会自动发布 `StarWar-windows-x64.zip` 到 Releases

## 项目结构

- `main/` – 游戏主程序与玩法逻辑
- `draw2d/` – 软件光栅化与绘制能力
- `support/` – 运行时/配置辅助模块
- `vmlib/` – 数学库（`Vec2f`, `Mat22f`）
- `lines-test/`, `triangles-test/` – 单元测试
- `blit-benchmark/`, `lines-benchmark/` – 基准测试
- `third_party/` – 第三方依赖

## 第三方许可证

详见 `third_party.md` 及 `third_party/` 目录下相关文件。

## 许可证

本项目采用 MIT License，见 `LICENSE`。
