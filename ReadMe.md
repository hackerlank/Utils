## 一个跨平台的 C/C++ 工具库

### 　　　　　　　　　　　　——　简洁、高效、实用

对于应用程序来说，除了业务逻辑相关的部分，往往都还需要处理一些基本的、常见的问题，如字符编码、运行日志、多线程等。

如果需要的这些功能都使用第三方的组件，那么难免会有“杀鸡焉用牛刀”的感觉。ICU库虽好，但如果仅仅用来转换字符编码会使软件安装包增加十几兆；BOOST库虽然强大，但如果仅仅需要一个跨平台的多线程库就去编译庞大的BOOST库，的确是大材小用，而且仅仅适用于C++程序。如果每个应用程序都自行实现这些功能组件，又难免会有“重复造轮子”的嫌疑，而且会增加开发的时间和成本，无法将精力集中在业务逻辑相关的部分。

作者也曾为上述问题困扰，但在开发过几个项目后，想到能否提取出其中的通用部分，形成一个通用的工具库？有了这个想法，本库就诞生并渐渐完善了起来。本库的实现有三个原则：简洁、高效、实用。

### 简洁

因为本库就是为了替代那些庞大的第三方库，因此实现必须简洁。简洁体现在几个方面：一是实现的都是常用的功能；二是代码的实现尽量简洁；三是磁盘文件简洁，主要功能都实现在cutil.h/c文件中；

### 高效

本库很少有复杂的内部实现，大多数是对系统调用和标准库的一层很薄的封装（Thin Wrapper）。对于一些需要内部实现的函数，大多也都选用标准的开源算法（如strlcpy），无法找到标准开源实现的，都经过多次测试和改进，力求代码实现简单，运行高效。

### 实用

本库支持跨平台（目前支持 Windows、Linux、Mac 平台），支持多种编译器（目前支持 MSVC、GCC、LLVM、MINGW），并且支持 32/64位编译。目前作者测试过的平台、编译器组合如下：

* slackware 13.1 32-bit + GCC 4.4.4 
* ubuntu 12.04 64-bit + GCC 4.6.3 
* WinXP 32-bit + VC 6.0
* Win7 32/64-bit + VS2010
* Qt Mingw g++
* Qt arm-linux-androideabi-g++
* Mac OS X 10.9 + Xcode 
* Mac OS X 10.9 + QtCreator

## 本库包括以下模块：

 * 基本数据类型
 * 数学函数和宏
 * 内存管理
 * 字符串操作
 * 文件系统
 * 时间和定时器
 * 字符编码
 * 多进程
 * 线程同步与互斥
 * 多线程
 * 日志系统
 * 内存检测(运行时)
 * 调试相关
 * 版本管理
 * 其他