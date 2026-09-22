# 资料与素材来源

本项目为经典 24 层魔塔的 9588 原生适配，非胖老鼠工作室官方移植。

- 原游戏：胖老鼠工作室《24 层魔塔》。
- 地图、怪物属性、道具数值与像素素材核对来源：[Mota Mod 的 24 层魔塔](https://motamod.com/games/24)，发布者大化术士，2026-09-22 获取。
- 使用的版本路径：`https://f641fb3d-a955-49c5-b019-c7bdcdf92b59.play.motamod.com/v/0bc66a1a-5f69-429b-9ba0-4a6cd2bfdcc5/`。
- `src/reference/` 保留地图、楼梯和属性的裁剪快照；不包含原版完整对话和 JS 游戏引擎。
- `src/assets/` 中 `animates.png`、`enemys.png`、`items.png`、`npcs.png`、`terrains.png` 来自该版本 `project/materials/materials.h5data`；`brave.png`、`ground.png`、`hero.png`、`icons.js` 来自其对应 `project/` 路径。
- `src/sprites.h` 由这些 32 像素素材最近邻转换为 20 像素 RGB565 格子，保留经典配色和轮廓。`assets/atlas-preview.png` 为转换检查图。
- 中文字模由本机 Windows 宋体在构建时栅格化为 12 像素子集。

游戏规则、原生 SDK 接入、UI、存档与测试在本目录实现。第三方游戏资料和素材的权利归原权利人，不因 SDK 的 Apache 许可证而改变。

剧情事件核对：2026-09-23读取上述版本 `project/data.js` 的 `firstData.startText` 与 `project/floors/MT0.js` 的仙子事件。该参考片头包含约10秒滚动叙事，仙子对白是独立事件。`src/story.h` 为本适配重新编写的分页文案，不含原版完整逐字对白。

## 许可边界

本仓库自编写的应用与构建/测试代码使用 Apache-2.0。经典地图、属性资料、像素素材及其生成表示（`src/data.h`、`src/sprites.h`）来自上述参考版本；当前没有取得可核验的独立开源素材许可证，不能将这些内容视为 Apache-2.0 授权。重新分发或其他用途所需权限应向原权利人核实。

`src/font.h` 是使用 Windows 宋体栅格化的有限字形数据，未附带宋体字体文件，也不声称重新许可微软字体。修改字模时可通过 `MOTA_FONT` 指定有权使用的字体；替换字体后需重新检查小屏排版。SDK 子模块及工具链依各自许可证使用。

因此，“开源”在这里指自编写代码的公开许可，不代表所有随附第三方内容都获得了同样授权。

应用图标 `assets/mota24-icon.png` 为本项目使用内置 imagegen 新生成的像素插画，非原版游戏素材；设计提示与处理方式见 `assets/icon-design.md`。
