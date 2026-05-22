# mini-mobile-client — 移动客户端 (C 语言实现)

## 概述

mini-mobile-client 是一个用 C99 编写的轻量级移动客户端框架，提供 React Native 风格的跨平台抽象层。包含面向移动端核心能力：JS/Native 双向通信桥、原生模块系统、Flexbox 布局引擎、高性能虚拟列表、以及完整的推送通知管理。

## 模块结构

| 模块 | 头文件 | 源文件 | 行数 |
|------|--------|--------|------|
| React Native Bridge | `include/react_native_bridge.h` | `src/react_native_bridge.c` | 130+ |
| Native Modules | `include/native_modules.h` | `src/native_modules.c` | 130+ |
| Yoga Layout | `include/yoga_layout.h` | `src/yoga_layout.c` | 130+ |
| List Virtualizer | `include/list_virtualizer.h` | `src/list_virtualizer.c` | 130+ |
| Push Notifications | `include/push_notif.h` | `src/push_notif.c` | 130+ |

## 快速开始

```bash
make
```

```c
#include "react_native_bridge.h"
#include "native_modules.h"
#include "yoga_layout.h"
#include "list_virtualizer.h"
#include "push_notif.h"

int main(void) {
    rn_bridge_t bridge;
    rn_bridge_init(&bridge);

    nm_registry_t registry;
    nm_registry_init(&registry);

    yg_node_t *root = yg_node_create();
    yg_node_calculate_layout(root, 375.0f, 812.0f, YG_DIRECTION_LTR);

    lv_controller_t list;
    lv_init(&list);

    pn_manager_t push;
    pn_init(&push);

    return 0;
}
```

## 构建

```bash
make          # 编译库文件
make examples # 编译示例程序
make demos    # 编译演示程序
make clean    # 清理构建产物
```

## 示例

- `examples/example1_bridge.c` — Bridge 消息收发示例
- `examples/example2_modules.c` — 原生模块注册与调用
- `examples/example3_layout.c` — Flexbox 布局计算

## 演示

- `demos/demo1_basic.c` — 综合基础演示 (250+ 行)
- `demos/demo2_full.c` — 完整功能演示 (250+ 行)

## 文档

- [API 参考](docs/API_REFERENCE.md)
- [设计文档](docs/DESIGN.md)

## 许可

MIT License
