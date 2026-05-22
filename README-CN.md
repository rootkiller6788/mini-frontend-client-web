# Mini Frontend Client Web（迷你前端客户端网页）

**从零开始、零依赖的 C 语言实现**，涵盖前端 Web 技术、浏览器引擎、UI 框架和移动客户端架构。每个模块以教学级精度建模真实前端系统行为 — 从 DOM 渲染到虚拟 DOM Diff、打包器 Tree-Shaking、状态管理和原生移动桥接。

## 模块总览

| 模块 | 主题 | 参考标准 |
|--------|--------|----------------|
| [mini-web-frontend](mini-web-frontend/) | HTML 解析器、CSS 解析器 + 层叠、DOM 树、布局引擎（Flexbox 仿真）、字体渲染模型 | Chromium Blink, W3C Specs |
| [mini-browser-base](mini-browser-base/) | HTTP 缓存（ETag/Last-Modified）、Cookie/Storage（Local/Session）、事件系统、Fetch/XHR 模型 | MDN, WhatWG Fetch |
| [mini-browser-core](mini-browser-core/) | 渲染管线（样式→布局→绘制→合成）、事件循环（宏任务/微任务）、V8 Isolate 仿真、浏览器 GC | Chrome V8, Chromium Docs |
| [mini-frontend-engineering](mini-frontend-engineering/) | 模块打包器（Webpack 仿真）、Tree-Shaking（死代码消除）、HMR（热模块替换）、SSR/SSG 模型 | Webpack, Vite, Next.js |
| [mini-frontend-framework](mini-frontend-framework/) | 虚拟 DOM（创建/Diff/Patch）、React 风格 Hooks（useState/useEffect）、组件模型、JSX 风格转译器仿真、SPA 路由 | React, Vue, SolidJS |
| [mini-state-data](mini-state-data/) | Redux Store（Actions/Reducers/Middleware）、MobX Observable、Zustand 风格 Store、React Query 缓存仿真、不可变更新 | Redux, Zustand, React Query |
| [mini-mobile-client](mini-mobile-client/) | React Native Bridge（JS↔Native）、原生模块注册、Flexbox Yoga 仿真、列表虚拟化、推送通知模型 | React Native, Flutter |
| [mini-ui-ux-hci](mini-ui-ux-hci/) | Design Token 系统、组件库目录、无障碍树（ARIA）、手势/点击模型、暗色模式主题 | Material Design, Ant Design |

## 设计理念

- **零外部依赖** — 纯 C（C99/C11），仅使用 `libc` 和 `libm`
- **模块自包含** — 每个目录自带 `Makefile`、`include/`、`src/`、`examples/`、`demos/`、`tests/`
- **用户态前端仿真** — 对浏览器引擎、前端框架和移动客户端的教学级建模
- **理论到代码的映射** — 每个模块包含 `docs/` 目录，内有参考规范对齐说明
- **实用演示程序** — 虚拟 DOM 引擎、CSS 布局引擎、模块打包器、React Native Bridge 仿真器等

## 构建方式

每个模块相互独立。进入模块目录后运行：

```bash
cd mini-frontend-framework
make all    # 构建全部
make test   # 运行测试
```

需要 **GCC** 和 **GNU Make**。

## 项目结构

```
mini-frontend-client-web/
├── mini-web-frontend/           # Web 前端基础
├── mini-browser-base/           # 浏览器基础能力
├── mini-browser-core/           # 浏览器核心引擎
├── mini-frontend-engineering/   # 前端工程化
├── mini-frontend-framework/     # 前端框架
├── mini-state-data/             # 状态管理与数据
├── mini-mobile-client/          # 移动客户端
└── mini-ui-ux-hci/              # UI/UX 与人机交互
```

## 许可证

MIT
