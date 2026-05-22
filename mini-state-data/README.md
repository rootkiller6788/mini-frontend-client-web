# mini-state-data — 状态与数据 (C 语言实现)

C99 实现的现代前端状态管理与数据缓存库集合。

## 概述

本库包含五大模块，涵盖了现代 Web 前端开发中最常用的状态管理范式：

| 模块 | 对标 | 描述 |
|------|------|------|
| `redux_store.h` | Redux | 单一状态树、纯函数 reducer、中间件链、时间旅行调试 |
| `mobx_observable.h` | MobX | 可观察状态、自动追踪、计算值、事务批量更新 |
| `zustand_sim.h` | Zustand | 轻量级 store、选择器订阅、不可变更新、中间件 |
| `query_cache.h` | TanStack Query | 查询缓存、自动重取、重试退避、乐观更新 |
| `immutable_store.h` | Immutable.js/Immer | 结构共享、草稿提交、差量补丁、正规化存储 |

## 构建

```sh
make
```

或手动编译：

```sh
gcc -std=c99 -Wall -Wextra -O2 -c redux_store.c -o redux_store.o
gcc -std=c99 -Wall -Wextra -O2 -c mobx_observable.c -o mobx_observable.o
gcc -std=c99 -Wall -Wextra -O2 -c zustand_sim.c -o zustand_sim.o
gcc -std=c99 -Wall -Wextra -O2 -c query_cache.c -o query_cache.o
gcc -std=c99 -Wall -Wextra -O2 -c immutable_store.c -o immutable_store.o
```

## 示例

- `example_todo.c` — 待办事项应用 (综合演示所有模块)
- `example_async.c` — 异步数据获取与缓存策略
- `example_collab.c` — 协同编辑状态 (CRDT 基础)

## 演示

- `demo_redux.c` — Redux 时间旅行调试完整演示 (250+ 行)
- `demo_query.c` — 查询缓存完整管线演示 (250+ 行)

## 文档

- `doc_api.md` — API 参考手册
- `doc_guide.md` — 使用指南与设计模式

## 许可证

MIT
