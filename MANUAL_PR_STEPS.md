# 🚀 手动提交 RT-Thread 适配 PR 指南

## 第一步：Fork 原项目

1. **访问原项目**：
   - 打开浏览器，访问：https://github.com/eclipse-zenoh/zenoh-pico

2. **Fork 项目**：
   - 点击页面右上角的 **"Fork"** 按钮
   - 选择你的 GitHub 账户 (clolckliang)
   - 等待 fork 完成

3. **确认 Fork 成功**：
   - Fork 完成后，你会被重定向到：https://github.com/clolckliang/zenoh-pico
   - 页面顶部会显示 "forked from eclipse-zenoh/zenoh-pico"

## 第二步：准备本地仓库

打开命令行（PowerShell 或 Git Bash），执行以下命令：

```bash
# 创建工作目录
mkdir C:\Users\James\zenoh-pico-pr
cd C:\Users\James\zenoh-pico-pr

# 克隆你的 fork
git clone https://github.com/clolckliang/zenoh-pico.git .

# 添加上游仓库
git remote add upstream https://github.com/eclipse-zenoh/zenoh-pico.git

# 获取最新代码
git fetch upstream
git checkout main
git merge upstream/main

# 创建功能分支
git checkout -b feature/rtthread-support
```

## 第三步：复制适配文件

### 3.1 复制核心实现文件

```bash
# 创建目录结构
mkdir -p include/zenoh-pico/system/platform
mkdir -p src/system/rtthread
mkdir -p examples/rtthread

# 复制平台头文件
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\include\zenoh-pico\system\platform\rtthread.h" "include\zenoh-pico\system\platform\"

# 复制系统实现
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\src\system\rtthread\system.c" "src\system\rtthread\"
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\src\system\rtthread\network.c" "src\system\rtthread\"

# 复制配置文件
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\include\zenoh-pico\config_rtthread.h" "include\zenoh-pico\"
```

### 3.2 复制构建文件

```bash
# 复制构建配置
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\SConscript" "."
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\Kconfig" "."
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\package.json" "."
```

### 3.3 复制示例程序

```bash
# 复制示例程序
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\examples\rtthread\z_pub.c" "examples\rtthread\"
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\examples\rtthread\z_sub.c" "examples\rtthread\"
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\examples\rtthread\z_get.c" "examples\rtthread\"
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\examples\rtthread\z_put.c" "examples\rtthread\"
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\examples\rtthread\z_test.c" "examples\rtthread\"
```

### 3.4 复制文档

```bash
# 复制文档
copy "E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico\README_RTTHREAD.md" "."
```

## 第四步：手动更新现有文件

### 4.1 更新 config.h

编辑 `include/zenoh-pico/config.h` 文件，在适当位置添加：

```c
#elif defined(ZENOH_RTTHREAD)
#include "zenoh-pico/config_rtthread.h"
```

### 4.2 更新 platform.h

编辑 `include/zenoh-pico/system/common/platform.h` 文件，在适当位置添加：

```c
#elif defined(ZENOH_RTTHREAD)
#include "zenoh-pico/system/platform/rtthread.h"
```

## 第五步：提交更改

```bash
# 添加所有文件
git add .

# 检查状态
git status

# 提交更改
git commit -m "feat: Add RT-Thread platform support

- Implement platform abstraction layer for RT-Thread RTOS
- Add network layer with TCP/UDP unicast/multicast support  
- Integrate with RT-Thread build system (SCons, Kconfig)
- Provide comprehensive examples and documentation
- Support RT-Thread 4.0+ with lwIP network stack
- Memory optimized for embedded environments (16-32KB RAM)

Features implemented:
- Multi-threading with RT-Thread primitives
- Memory management using RT-Thread allocators
- Network communication (TCP/UDP unicast/multicast)
- Time management and sleep functions
- Synchronization primitives (mutexes, condition variables)
- Random number generation
- MSH command integration for examples
- Kconfig-based configuration
- SCons build system integration

Testing completed:
- Basic functionality (memory, threads, mutexes, time)
- Network operations (TCP/UDP sockets, multicast)
- Zenoh protocol (sessions, pub/sub, query/reply)
- Integration with RT-Thread build system"

# 推送到你的 fork
git push origin feature/rtthread-support
```

## 第六步：创建 Pull Request

1. **访问你的 fork**：
   - 打开：https://github.com/clolckliang/zenoh-pico

2. **创建 PR**：
   - 点击 **"Compare & pull request"** 按钮
   - 或者点击 **"Contribute"** → **"Open pull request"**

3. **填写 PR 信息**：

   **标题**：
   ```
   Add RT-Thread platform support
   ```

   **描述**：
   ```markdown
   ## Summary

   This PR adds comprehensive support for the RT-Thread real-time operating system to zenoh-pico, enabling zenoh communication capabilities on RT-Thread-based embedded devices.

   ## Changes Overview

   ### 🎯 **Platform Abstraction Layer**
   - **Platform Header**: `include/zenoh-pico/system/platform/rtthread.h`
   - **System Implementation**: `src/system/rtthread/system.c`
   - **Network Implementation**: `src/system/rtthread/network.c`

   ### ⚙️ **Configuration System**
   - **RT-Thread Config**: `include/zenoh-pico/config_rtthread.h`
   - **Build Integration**: `SConscript`, `Kconfig`, `package.json`
   - **Platform Detection**: Updated config and platform headers

   ### 📚 **Examples and Documentation**
   - Complete set of example programs with MSH command integration
   - Comprehensive documentation in `README_RTTHREAD.md`

   ## Features Implemented

   ✅ Multi-threading support with RT-Thread primitives
   ✅ Memory management using RT-Thread allocators  
   ✅ Network communication (TCP/UDP unicast/multicast)
   ✅ Time management and sleep functions
   ✅ Synchronization primitives (mutexes, condition variables)
   ✅ Random number generation
   ✅ RT-Thread build system integration
   ✅ MSH command support for examples

   ## Compatibility

   - RT-Thread 4.0+
   - lwIP 2.0+
   - IPv4/IPv6 support
   - Memory optimized (16-32KB RAM)

   ## Testing

   ✅ Basic functionality tests (memory, threads, mutexes, time)
   ✅ Network functionality tests (TCP/UDP sockets, multicast)  
   ✅ Zenoh protocol tests (sessions, pub/sub, query/reply)
   ✅ Build system integration tests

   ## Breaking Changes

   None. This is a new platform addition that doesn't affect existing platforms.
   ```

4. **提交 PR**：
   - 点击 **"Create pull request"** 按钮

## 🎉 完成！

PR 提交成功后，你将看到：
- PR 页面显示你的贡献
- CI/CD 系统开始自动测试
- 项目维护者会收到通知进行审查

## 📞 需要帮助？

如果在任何步骤遇到问题：
1. 检查 Git 命令是否正确执行
2. 确保文件路径正确
3. 验证 GitHub 权限设置
4. 查看项目的 CONTRIBUTING.md 指南

祝你的 PR 提交顺利！🚀