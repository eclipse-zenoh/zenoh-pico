# Zenoh-Pico for RT-Thread

本文档介绍如何在RT-Thread操作系统中使用zenoh-pico库。

## 概述

zenoh-pico是Eclipse zenoh的C语言客户端库，专为资源受限的嵌入式设备设计。本适配版本支持在RT-Thread实时操作系统上运行。

## 特性

- 支持发布/订阅模式
- 支持查询/应答模式
- 支持TCP、UDP单播和多播传输
- 多线程支持
- 内存管理优化
- 网络连接管理

## 系统要求

- RT-Thread 4.0+
- 支持POSIX socket API
- 支持lwIP网络协议栈
- 至少32KB RAM（取决于配置）

## 编译配置

### 1. 添加到RT-Thread项目

将zenoh-pico目录复制到RT-Thread项目的packages目录下，或者作为子模块添加。

### 2. 配置选项

在menuconfig中启用zenoh-pico：

```
RT-Thread online packages
    IoT - internet of things
        [*] zenoh-pico: Eclipse zenoh C client library
            [*] Enable multi-threading support
            [*] Enable publication support
            [*] Enable subscription support
            [*] Enable query support
            [*] Enable queryable support
            [*] Enable TCP transport
            [*] Enable UDP unicast transport
            [*] Enable UDP multicast transport
            [ ] Enable serial transport
            (0) Debug level
```

### 3. 编译定义

确保在编译时定义以下宏：

```c
#define ZENOH_RTTHREAD
#define Z_FEATURE_MULTI_THREAD 1
```

## 使用示例

### 发布者示例

```c
#include "zenoh-pico.h"

int zenoh_publisher_example(void) {
    z_owned_config_t config;
    z_config_default(&config);
    
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }
    
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, "demo/example");
    z_declare_publisher(&pub, z_loan(s), z_loan(ke), NULL);
    
    for (int i = 0; i < 10; i++) {
        char buf[256];
        sprintf(buf, "[%d] Hello from RT-Thread!", i);
        z_owned_bytes_t payload;
        z_bytes_from_str(&payload, buf);
        z_publisher_put(z_loan(pub), z_move(payload), NULL);
        rt_thread_mdelay(1000);
    }
    
    z_undeclare_publisher(z_move(pub));
    z_close(z_move(s), NULL);
    return 0;
}
```

### 订阅者示例

```c
#include "zenoh-pico.h"

void data_handler(z_loaned_sample_t *sample, void *ctx) {
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);
    printf("Received: %s\n", z_string_data(z_loan(value)));
    z_drop(z_move(value));
}

int zenoh_subscriber_example(void) {
    z_owned_config_t config;
    z_config_default(&config);
    
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }
    
    z_owned_closure_sample_t callback;
    z_closure(&callback, data_handler, NULL, NULL);
    
    z_owned_subscriber_t sub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, "demo/example");
    z_declare_subscriber(&sub, z_loan(s), z_loan(ke), z_move(callback), NULL);
    
    // 保持运行
    while (1) {
        rt_thread_mdelay(1000);
    }
    
    z_undeclare_subscriber(z_move(sub));
    z_close(z_move(s), NULL);
    return 0;
}
```

## MSH命令

适配版本提供了以下MSH命令用于测试：

- `zenoh_pub` - 运行发布者示例
- `zenoh_sub` - 运行订阅者示例  
- `zenoh_get` - 运行GET查询示例
- `zenoh_put` - 运行PUT示例

## 网络配置

确保RT-Thread系统已正确配置网络：

1. 启用lwIP网络协议栈
2. 配置正确的IP地址
3. 确保网络连接正常

## 内存使用

zenoh-pico的内存使用取决于配置：

- 基本配置：约16KB RAM
- 完整配置：约32KB RAM
- 可通过禁用不需要的功能来减少内存使用

## 故障排除

### 编译错误

1. 确保包含了正确的头文件路径
2. 检查ZENOH_RTTHREAD宏是否已定义
3. 确保RT-Thread版本兼容

### 运行时错误

1. 检查网络连接
2. 验证zenoh路由器是否运行
3. 检查内存是否足够
4. 启用调试输出查看详细信息

### 网络问题

1. 检查IP配置
2. 验证防火墙设置
3. 确认端口7447可用（zenoh默认端口）

## 性能优化

1. 根据需要调整缓冲区大小
2. 禁用不需要的传输协议
3. 优化网络参数
4. 使用适当的线程优先级

## 许可证

本适配遵循zenoh-pico的原始许可证：EPL-2.0 OR Apache-2.0

## 支持

如有问题，请参考：
- [zenoh官方文档](https://zenoh.io/)
- [zenoh-pico GitHub](https://github.com/eclipse-zenoh/zenoh-pico)
- [RT-Thread官方文档](https://www.rt-thread.org/)