# lab0

## reference link

- [lab0实验手册](https://cs144.github.io/assignments/check0.pdf)

## implementation

### Set up GNU/Linux on your computer

简单地安装`CS144 VirtualBox`和c++环境，以方便后续地测试。

### Networking by hand

接下来就是使用简单地`telnet`来构造HTTP请求和SMTP请求，并得到对应的响应。
（亲手写出一部分报文

### Listening and connecting

通过`netcat`建立一个全双工通信的服务端，感受`netcat`的使用。
（类似于即时通讯服务

### Writing a network program

> by using an OS stream socket

#### part 1

lab0的最后一项任务就是使用原生的tcp socket来编写http报文结构，从而在socket的基础上发起一次http请求
详见`lab0/minow/apps/webget.cc`代码实现。

#### part2

part2实现可靠字节流，使用一个普通string来存储字节流
***

# lab1

## reference link

- [lab1实验手册](https://cs144.github.io/assignments/check1.pdf)
- [博客](https://hangx-ma.github.io/2023/05/14/cs144-lab1.html)
## implementation
在tcp/ip协议栈中，数据传输的可靠性不是网络来承担的，而是交由端系统来承担的。网络中的有序字节流从一端发送到另一端，跨越了诸多网络的路由器
，期间难免出现字节流传输的乱序、重复、重叠等。而lab1就是需要在这样不可靠字节流中去建立可靠的字节流。
### 具体实现
这里参考了上面的博客
![Memory usage limitation of Reassembler and ByteStream](https://hangx-ma.github.io/norobots/images/2023-05-14-cs144-lab1/bytestream_space_interpretation.png)