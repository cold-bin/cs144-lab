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

这里参考了上面的博客<br>
![Memory usage limitation of Reassembler and ByteStream](static/img-unassembled.png)

## lab2

## reference link

## implementation

### `wrap`与`unwrap`

![seqno](static/img-seqno.png)

- `wrap`是将`absolute seqno`转化为`seqno` <br>
  由于`absolute seqno`是非循环序号，`seqno`是循环序号，所以需要取模转化。（当然也可以直接截断。

- `unwrap`是将`seqno`转化为`absolute seqno` <br>
  > `checkpoint`其实就是`first_unassembled_index`

  这里的处理比较麻烦。我最开始的想法是，循环找出最小的`checkpoint - (seqno+x * 2^32)`，也就是离得最近的`x`。
  但是复杂度比较高，看了大佬的博客，利用位运算，可以将`O(x)`的时间复杂度降低为O(1). <br>
  官方让我们找到离`checkpoint`最近的`absolute seqno`，因为给出`seqno`，会有多个`absolute seqno`与之对应，

### TCP Receiver
