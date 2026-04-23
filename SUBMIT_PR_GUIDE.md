# 向 zenoh-pico 提交 RT-Thread 支持的 PR 指南

## 🎯 概述

本指南将帮助你向 Eclipse zenoh-pico 项目提交 RT-Thread 平台支持的 Pull Request。

## 📋 准备工作

### 1. 确认所有文件已创建

请确保以下文件都已正确创建：

#### 核心实现文件
- ✅ `include/zenoh-pico/system/platform/rtthread.h`
- ✅ `src/system/rtthread/system.c`
- ✅ `src/system/rtthread/network.c`
- ✅ `include/zenoh-pico/config_rtthread.h`

#### 配置集成文件
- ✅ `SConscript`
- ✅ `Kconfig`
- ✅ `package.json`
- ✅ 已修改 `include/zenoh-pico/config.h`
- ✅ 已修改 `include/zenoh-pico/system/platform.h`

#### 示例程序
- ✅ `examples/rtthread/z_pub.c`
- ✅ `examples/rtthread/z_sub.c`
- ✅ `examples/rtthread/z_get.c`
- ✅ `examples/rtthread/z_put.c`
- ✅ `examples/rtthread/z_test.c`

#### 文档
- ✅ `README_RTTHREAD.md`

## 🚀 提交步骤

### 第一步：Fork 原项目

1. 访问 https://github.com/eclipse-zenoh/zenoh-pico
2. 点击右上角的 "Fork" 按钮
3. 选择你的 GitHub 账户

### 第二步：克隆你的 Fork

```bash
# 克隆你的 fork
git clone https://github.com/YOUR_USERNAME/zenoh-pico.git
cd zenoh-pico

# 添加上游仓库
git remote add upstream https://github.com/eclipse-zenoh/zenoh-pico.git

# 确保是最新的代码
git fetch upstream
git checkout main
git merge upstream/main
```

### 第三步：创建功能分支

```bash
git checkout -b feature/rtthread-support
```

### 第四步：复制适配文件

将你创建的所有 RT-Thread 适配文件复制到克隆的仓库中：

```bash
# 复制平台头文件
cp /path/to/your/rtthread.h include/zenoh-pico/system/platform/

# 复制系统实现
mkdir -p src/system/rtthread
cp /path/to/your/system.c src/system/rtthread/
cp /path/to/your/network.c src/system/rtthread/

# 复制配置文件
cp /path/to/your/config_rtthread.h include/zenoh-pico/

# 复制构建文件
cp /path/to/your/SConscript .
cp /path/to/your/Kconfig .
cp /path/to/your/package.json .

# 复制示例程序
mkdir -p examples/rtthread
cp /path/to/your/examples/rtthread/* examples/rtthread/

# 复制文档
cp /path/to/your/README_RTTHREAD.md .

# 手动更新这两个文件（添加 RT-Thread 支持）
# include/zenoh-pico/config.h
# include/zenoh-pico/system/platform.h
```

### 第五步：提交更改

```bash
# 添加所有新文件
git add include/zenoh-pico/system/platform/rtthread.h
git add include/zenoh-pico/config_rtthread.h
git add src/system/rtthread/
git add examples/rtthread/
git add SConscript Kconfig package.json
git add README_RTTHREAD.md

# 添加修改的文件
git add include/zenoh-pico/config.h
git add include/zenoh-pico/system/platform.h

# 提交
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
```

### 第六步：推送到你的 Fork

```bash
git push origin feature/rtthread-support
```

### 第七步：创建 Pull Request

1. 访问你的 fork：`https://github.com/YOUR_USERNAME/zenoh-pico`
2. 点击 "Compare & pull request" 按钮
3. 填写 PR 信息：

#### PR 标题
```
Add RT-Thread platform support
```

#### PR 描述
使用 `PR_DESCRIPTION.md` 中的内容，或者使用以下简化版本：

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

4. 点击 "Create pull request"

## 📝 提交后的注意事项

### 1. 监控 PR 状态
- 关注 CI/CD 构建状态
- 查看是否有构建失败
- 及时响应维护者的反馈

### 2. 响应审查意见
- 仔细阅读审查者的建议
- 及时进行必要的修改
- 保持礼貌和专业的沟通

### 3. 可能的修改要求
- 代码风格调整
- 文档改进
- 测试用例补充
- 性能优化建议

### 4. 更新 PR
如果需要修改：

```bash
# 在你的功能分支上进行修改
git add modified_files
git commit -m "fix: address review comments"
git push origin feature/rtthread-support
```

## 🎉 成功合并后

1. **同步你的 fork**：
```bash
git checkout main
git fetch upstream
git merge upstream/main
git push origin main
```

2. **删除功能分支**：
```bash
git branch -d feature/rtthread-support
git push origin --delete feature/rtthread-support
```

3. **庆祝贡献**！🎉

## 📞 需要帮助？

如果在提交过程中遇到问题：

1. 查看 zenoh-pico 项目的 CONTRIBUTING.md
2. 在 GitHub Issues 中搜索相关问题
3. 在 zenoh 社区论坛寻求帮助
4. 联系项目维护者

## 🔍 最终检查清单

在提交 PR 之前，请确保：

- [ ] 所有文件都已正确复制
- [ ] 代码编译无错误
- [ ] 示例程序可以运行
- [ ] 文档完整且准确
- [ ] 提交信息清晰明了
- [ ] PR 描述详细且专业
- [ ] 没有破坏现有功能
- [ ] 遵循项目的编码规范

祝你的 PR 提交顺利！这将是对 zenoh-pico 项目的重要贡献。🚀