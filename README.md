## 猫玉

一个简单小巧的开源的非想天则对战平台。

### 原理

通过APIHOOK实现对游戏中UDP数据的转发（需要中转服务器），并通过COM HOOK实现游戏内部分数据的绘图。

因此客户端理论上适应任意使用UDP进行联机的游戏。

服务端采用简易的select单线程模型编写，因此效率有待提升。

### 编译

#### 客户端编译要求(Windows only)

+ VisualStudio 2013
+ DirectX SDK

#### 服务端编译要求(Windows/linux)

+ Windows: VisualStudio 2013
+ Linux: CodeBlocks且g++版本不低于4.8

### 部署

#### 客户端运行要求

+ 非想天则

+ DirectX Runtime

#### 客户端配置

按下述说明修改nekotama.conf：

```
    game=D:\Touhou\th123\th123.exe  # 游戏路径
    args=                           # 游戏运行附加参数
    workdir=D:\Touhou\th123\        # 游戏运行目录
    server=127.0.0.1                # 对战平台服务器地址
    port=12801                      # 游戏端口
    nickname=chu                    # 昵称
```

### 许可

BSD Lisence
