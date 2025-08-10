# Git Commit Messages for RT-Thread Support PR

## Main Commit Message

```
feat: Add RT-Thread platform support

- Implement platform abstraction layer for RT-Thread RTOS
- Add network layer with TCP/UDP unicast/multicast support  
- Integrate with RT-Thread build system (SCons, Kconfig)
- Provide comprehensive examples and documentation
- Support RT-Thread 4.0+ with lwIP network stack
- Memory optimized for embedded environments (16-32KB RAM)

Closes: #[issue_number_if_exists]
```

## Individual Commit Messages (if splitting into multiple commits)

### 1. Platform Abstraction Layer
```
feat(platform): Add RT-Thread platform abstraction layer

- Add platform header with RT-Thread specific types
- Implement system functions (threads, mutexes, time, memory)
- Support RT-Thread threading and synchronization primitives
- Add random number generation using rt_tick_get()

Files:
- include/zenoh-pico/system/platform/rtthread.h
- src/system/rtthread/system.c
```

### 2. Network Implementation
```
feat(network): Add RT-Thread network implementation

- Implement TCP socket operations with lwIP integration
- Add UDP unicast and multicast support
- Support IPv4/IPv6 with standard POSIX socket API
- Handle non-blocking I/O and socket timeouts

Files:
- src/system/rtthread/network.c
```

### 3. Configuration System
```
feat(config): Add RT-Thread configuration support

- Add RT-Thread specific configuration header
- Integrate with RT-Thread Kconfig system
- Add SCons build script for RT-Thread projects
- Update platform detection logic

Files:
- include/zenoh-pico/config_rtthread.h
- include/zenoh-pico/config.h (modified)
- include/zenoh-pico/system/platform.h (modified)
- SConscript
- Kconfig
- package.json
```

### 4. Examples and Documentation
```
feat(examples): Add RT-Thread examples and documentation

- Add publisher, subscriber, get, put examples
- Implement MSH command integration
- Add basic functionality test program
- Provide comprehensive usage documentation

Files:
- examples/rtthread/z_pub.c
- examples/rtthread/z_sub.c
- examples/rtthread/z_get.c
- examples/rtthread/z_put.c
- examples/rtthread/z_test.c
- README_RTTHREAD.md
```

## Git Commands Sequence

```bash
# 1. Create a new branch for RT-Thread support
git checkout -b feature/rtthread-support

# 2. Add all RT-Thread related files
git add include/zenoh-pico/system/platform/rtthread.h
git add include/zenoh-pico/config_rtthread.h
git add src/system/rtthread/
git add examples/rtthread/
git add SConscript Kconfig package.json
git add README_RTTHREAD.md

# 3. Add modified files
git add include/zenoh-pico/config.h
git add include/zenoh-pico/system/platform.h

# 4. Commit with main message
git commit -m "feat: Add RT-Thread platform support

- Implement platform abstraction layer for RT-Thread RTOS
- Add network layer with TCP/UDP unicast/multicast support  
- Integrate with RT-Thread build system (SCons, Kconfig)
- Provide comprehensive examples and documentation
- Support RT-Thread 4.0+ with lwIP network stack
- Memory optimized for embedded environments (16-32KB RAM)"

# 5. Push to your fork
git push origin feature/rtthread-support
```

## Alternative: Split into Multiple Commits

```bash
# If you prefer multiple smaller commits:

# Commit 1: Platform abstraction
git add include/zenoh-pico/system/platform/rtthread.h src/system/rtthread/system.c
git commit -m "feat(platform): Add RT-Thread platform abstraction layer"

# Commit 2: Network implementation  
git add src/system/rtthread/network.c
git commit -m "feat(network): Add RT-Thread network implementation"

# Commit 3: Configuration
git add include/zenoh-pico/config_rtthread.h include/zenoh-pico/config.h include/zenoh-pico/system/platform.h SConscript Kconfig package.json
git commit -m "feat(config): Add RT-Thread configuration support"

# Commit 4: Examples and docs
git add examples/rtthread/ README_RTTHREAD.md
git commit -m "feat(examples): Add RT-Thread examples and documentation"
```

## PR Creation Steps

1. **Fork the Repository**:
   ```bash
   # Go to https://github.com/eclipse-zenoh/zenoh-pico
   # Click "Fork" button
   ```

2. **Clone Your Fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/zenoh-pico.git
   cd zenoh-pico
   ```

3. **Add Upstream Remote**:
   ```bash
   git remote add upstream https://github.com/eclipse-zenoh/zenoh-pico.git
   ```

4. **Create Feature Branch**:
   ```bash
   git checkout -b feature/rtthread-support
   ```

5. **Copy Your Changes**:
   ```bash
   # Copy all the RT-Thread adaptation files to the cloned repository
   ```

6. **Commit and Push**:
   ```bash
   # Use the commit messages above
   git push origin feature/rtthread-support
   ```

7. **Create Pull Request**:
   - Go to your fork on GitHub
   - Click "New Pull Request"
   - Use the PR description from PR_DESCRIPTION.md
   - Submit the PR

## Important Notes

- Make sure to test your changes before submitting
- Follow the project's coding standards
- Include comprehensive documentation
- Ensure no breaking changes to existing platforms
- Be responsive to review feedback