# CSAPP Lab -- Bomb Lab
<!-- TOC -->

- [CSAPP Lab -- Bomb Lab](#csapp-lab----bomb-lab)
    - [准备](#)
    - [开始](#)
    - [bomb 1](#bomb-1)
    - [bomb 2](#bomb-2)
    - [bomb 3](#bomb-3)
    - [bomb 4](#bomb-4)
    - [bomb 5](#bomb-5)
    - [bomb 6](#bomb-6)

<!-- /TOC -->
## 准备
这个实验是通过反汇编一个可执行文件，分析汇编代码来找到六个炸弹的密码。完成实验需要熟悉一些比较基础的汇编知识，掌握GDB的使用。然后就只需要耐心读汇编，通过GDB查看有用信息，就可以完成这个lab了。

关于GDB，可以参考[这个教程](http://beej.us/guide/bggdb/)。另外，还需要使用`objdump -d ./bomb >> bomb.s`反汇编工具来得到汇编代码。

## 开始
首先定位到`main`函数对应的机器代码。分析这段代码，可以看到前面都是和`bomb.c`中`main()`函数对应的。

main开头是一个if分支语句，机器代码对应的就是`cmp`和`jmp`。继续分析:
```C++
else {
    printf("Usage: %s [<input_file>]\n", argv[0]);
    exit(8);
}
```

我们会发现对应于这个else部分的机器代码是这样的:

```Assembly
400df8:	48 8b 16             	mov    (%rsi),%rdx
  400dfb:	be d3 22 40 00       	mov    $0x4022d3,%esi
  400e00:	bf 01 00 00 00       	mov    $0x1,%edi
  400e05:	b8 00 00 00 00       	mov    $0x0,%eax
  400e0a:	e8 f1 fd ff ff       	callq  400c00 <__printf_chk@plt>
  400e0f:	bf 08 00 00 00       	mov    $0x8,%edi
  400e14:	e8 07 fe ff ff       	callq  400c20 <exit@plt>
```
字符串常量会存放在代码的静态内存区，所以我们一般都会用一个指针来引用这个字符串常量。所以，只要我们知道了这个地址，就能够用gdb来print得到这个字符串常量。我们可以在这里验证一下，观察这段机器代码，可以确定`mov $0x4022d3,%esi`是在调用`printf("Usage: %s [<input_file>]\n", argv[0]);`这行代码时用到的。

gdb调试:
```
(gdb) p (char*)0x4022d3
$2 = 0x4022d3 "Usage: %s [<input_file>]\n"
```

结果验证了这个想法是正确的，所以我们可以利用这个思路来进行接下来的lab。


## bomb 1
通过阅读代码，找到bomb1对应的函数:

```
0000000000400ee0 <phase_1>:
  400ee0:	48 83 ec 08          	sub    $0x8,%rsp
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
  400ee9:	e8 4a 04 00 00       	callq  401338 <strings_not_equal>
  400eee:	85 c0                	test   %eax,%eax
  400ef0:	74 05                	je     400ef7 <phase_1+0x17>
  400ef2:	e8 43 05 00 00       	callq  40143a <explode_bomb>
  400ef7:	48 83 c4 08          	add    $0x8,%rsp
  400efb:	c3                   	retq   
```
根据我们上面的思路，很容易发现这里把`0x402400`加载进了%esi作为函数参数，然后调用了`string_not_equal`函数。所以，那个地址想必就是bomb1的key了。

GDB调试:
```
(gdb) print (char*)0x402400
$4 = 0x402400 "Border relations with Canada have never been better."
```

轻易找到答案，热身完毕。

## bomb 2
```
0000000000400efc <phase_2>:
  400efc:	55                   	push   %rbp
  400efd:	53                   	push   %rbx
  400efe:	48 83 ec 28          	sub    $0x28,%rsp
  400f02:	48 89 e6             	mov    %rsp,%rsi
  400f05:	e8 52 05 00 00       	callq  40145c <read_six_numbers>
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp)
  400f0e:	74 20                	je     400f30 <phase_2+0x34>
  400f10:	e8 25 05 00 00       	callq  40143a <explode_bomb>
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax
  400f1a:	01 c0                	add    %eax,%eax
  400f1c:	39 03                	cmp    %eax,(%rbx)
  400f1e:	74 05                	je     400f25 <phase_2+0x29>
  400f20:	e8 15 05 00 00       	callq  40143a <explode_bomb>
  400f25:	48 83 c3 04          	add    $0x4,%rbx
  400f29:	48 39 eb             	cmp    %rbp,%rbx
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b>
  400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40>
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx
  400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp
  400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>
  400f3c:	48 83 c4 28          	add    $0x28,%rsp
  400f40:	5b                   	pop    %rbx
  400f41:	5d                   	pop    %rbp
  400f42:	c3                   	retq  
```

找到`phase_2`，看到开始是用`sub $0x28, %rsp`分配了一个40bytes的栈空间，然后调用了函数`read_six_numbers`，根据这个函数的名字，猜想应该是从输入读取六个数字。

为了验证猜想，找到`read_six_numbers`，发现其中有一行`callq  400bf0 <__isoc99_sscanf@plt>`调用来库函数sscanf。并且，利用我们前面的技巧，打印出`$0x4025c3`地址的字符串：
```
(gdb) print (char*)0x4025c3
$3 = 0x4025c3 "%d %d %d %d %d %d"
```

可以确定，`phase_2`是调用了`read_six_numbers`来读取六个数字。因此，我们只需要找出这六个数，就可以拆除炸弹了。

继续分析`phase_2`:
```
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp)
  400f0e:	74 20                	je     400f30 <phase_2+0x34>
  400f10:	e8 25 05 00 00       	callq  40143a <explode_bomb>
```

这里比较了处于栈顶的数和`$0x1`，如果不相等就会explode，所以确定第一个数就是1。再根据je跳转，发现这里将`%rsp+0x4`和`%rsp+0x18`对应的值存入了寄存器，然后跳转到`400f17`，而这里将`%rbx-4`指向的数放入`%eax`后，对其乘以2再和`%rbx`对应的数进行比较，如果不相等就会引爆炸弹，否则就是一个循环。

至此，我们可以确定这里是一个循环，并且每次都是将`%rbx-4`对应的值乘以2和`%rbx`对应的值进行比较，也就是说，每次都是将栈中前一个数的两倍和后一个数比较。因此，可以确定这六个数是一个等比数列，并且第一个数是1。

所以，bomb2的key就是: 1 2 4 8 16 32

## bomb 3
```ass
0000000000400f43 <phase_3>:
  400f43:	48 83 ec 18          	sub    $0x18,%rsp
  400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi
  400f56:	b8 00 00 00 00       	mov    $0x0,%eax
  400f5b:	e8 90 fc ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  400f60:	83 f8 01             	cmp    $0x1,%eax
  400f63:	7f 05                	jg     400f6a <phase_3+0x27>
  400f65:	e8 d0 04 00 00       	callq  40143a <explode_bomb>
  
  400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp)
  400f6f:	77 3c                	ja     400fad <phase_3+0x6a>
  400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax
  400f75:	ff 24 c5 70 24 40 00 	jmpq   *0x402470(,%rax,8)

  400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
  400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
  400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
  400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  400f91:	b8 85 01 00 00       	mov    $0x185,%eax
  400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
  400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
  400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>
  400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
  400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>
  400fad:	e8 88 04 00 00       	callq  40143a <explode_bomb>
  400fb2:	b8 00 00 00 00       	mov    $0x0,%eax
  400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>
  400fb9:	b8 37 01 00 00       	mov    $0x137,%eax
  400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax
  400fc2:	74 05                	je     400fc9 <phase_3+0x86>
  400fc4:	e8 71 04 00 00       	callq  40143a <explode_bomb>
  400fc9:	48 83 c4 18          	add    $0x18,%rsp
  400fcd:	c3                   	retq   
```

开始调用了`sscanf`，按照我们之前的经验，可以用GDB调试器查看调用的参数，也就是数据输入的格式，根据`mov $0x4025cf,%esi`:
```
(gdb) print (char*)0x4025cf
$1 = 0x4025cf "%d %d"
```

可以得知我们输入的应该是两个整数，然后再继续下面的分析。先是%rax只有大于1，才不会爆炸。然后比较(%rsp+8)对应的值和0x7的大小关系，如果大于7就会爆炸，然后`jmpq *0x402470(,%rax,8)`进行跳转。这样的结构就是switch的特征！然后再结合后面的若干mov和jmp，可以断定这是一个switch结构，并且每次跳转后，都会比较一个常数和%eax中的值，如果相等就会结束，否则会爆炸。

所以，我们推断，输入的两个数中的第一个数应该是索引，然后第二个数就是索引对应的case中的数。

那么，我们假设查找第二个case，也就是jump table的address+8对应的那个case:
```
(gdb) print *(0x402470 + 0x8)
$2 = 4198329
```
如我们所料，会得到一个地址，然后转换成十六进制就是`400fb9`，而这个对应的就是:
`  400fb9:	b8 37 01 00 00       	mov    $0x137,%eax`。所以，答案中的第二个数应该就是`0x137`！转换成十进制就是311。

于是，我们得到了其中的一个答案就是: 1 311。

当然，还有其他的case都能够正确地通，如2 707等。


## bomb 4
```
000000000040100c <phase_4>:
  40100c:	48 83 ec 18          	sub    $0x18,%rsp
  401010:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  401015:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  40101a:	be cf 25 40 00       	mov    $0x4025cf,%esi
  40101f:	b8 00 00 00 00       	mov    $0x0,%eax
  401024:	e8 c7 fb ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  401029:	83 f8 02             	cmp    $0x2,%eax
  40102c:	75 07                	jne    401035 <phase_4+0x29>
  40102e:	83 7c 24 08 0e       	cmpl   $0xe,0x8(%rsp)
  401033:	76 05                	jbe    40103a <phase_4+0x2e>
  401035:	e8 00 04 00 00       	callq  40143a <explode_bomb>
  40103a:	ba 0e 00 00 00       	mov    $0xe,%edx
  40103f:	be 00 00 00 00       	mov    $0x0,%esi
  401044:	8b 7c 24 08          	mov    0x8(%rsp),%edi
  401048:	e8 81 ff ff ff       	callq  400fce <func4>
  40104d:	85 c0                	test   %eax,%eax
  40104f:	75 07                	jne    401058 <phase_4+0x4c>
  401051:	83 7c 24 0c 00       	cmpl   $0x0,0xc(%rsp)
  401056:	74 05                	je     40105d <phase_4+0x51>
  401058:	e8 dd 03 00 00       	callq  40143a <explode_bomb>
  40105d:	48 83 c4 18          	add    $0x18,%rsp
  401061:	c3                   	retq  
```

还是之前的套路，分析输入的应该是两个整数。然后，`phase_4`部分其实很简单，就是读取输入的两个数。然后第一个数作为参数(存入%edi，并且第一个参数小于等于14)，另外0xe和0x0作为第二个和第三个参数来调用函数`func4`。调用结束后，返回值(%eax)应该是0。根据`cmpl   $0x0,0xc(%rsp)`可知，第二个输入的数字也应该为0。

接下来，分析函数`func4`。由前面的分析，这个函数应该有三个参数，接下来只需要给每个寄存器一个名字，然后将汇编代码转换成逻辑图就行了:
```
设定: %edi -- a, %ecx -- b, %esi -- c, %edx -- d, %eax -- r
r = d - c
b = (d - c) >> 31
r = (r + b) => r = d < c ? (d - c + 1) : (d - c)
r >>= 1
b = r + c
if b <= a:
    r = 0
    if b >= a:
        return r
    else:
        r = func4(a, c, d)
        return 2*r+1
else:
    r = func4(a, c, b-1)
    return 2*r
```
再简化一下：
```
def func(a, c, d):
    b = d < c ? (d+c+1)/2 : (d+c)/2
    if b < a:
        return func(a, b+d, d)*2 + 1
    else if b > a:
        return func(a, c, b-1)*2
    else:
        return 0
```
根据这个逻辑，如果函数返回值为0，则是b == a的情况，而上面分析到了另外两个参数c = 0, d = 14，所以此时a = b = (d+c)/2 = 7。

所以，输入的第一个数应该是7!输入的第二个数应该是0!


## bomb 5
```
  40107a:	e8 9c 02 00 00       	callq  40131b <string_length>
  40107f:	83 f8 06             	cmp    $0x6,%eax
```
分析这部分，调用了`string_length`函数，并且要求返回值为6，所以输入的应该是一个字符串，而且长度为6。

```
  4010ae:	c6 44 24 16 00       	movb   $0x0,0x16(%rsp)
  4010b3:	be 5e 24 40 00       	mov    $0x40245e,%esi       
  4010b8:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi
  4010bd:	e8 76 02 00 00       	callq  401338 <strings_not_equal>
  4010c2:	85 c0                	test   %eax,%eax
  4010c4:	74 13                	je     4010d9 <phase_5+0x77>
```

这里分别以(%rsp+0x10到%rsp+0x16)的字符串和地址`0x40245e`的字符串作为参数，调用了函数`string_not_equal`。根据这段可以肯定，我们需要输入长度为6的字符串来和存放在地址`0x40245e`的字符串来进行比较，如果不同，就会Bomb。用GDB可以得到这个字符串是"flyers":
```
(gdb) print (char*)0x40245e
$1 = 0x40245e "flyers"
```

所以，现在我们的目的就很明确了，如果得到"flyers"。再分析这段:
```
  40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx
  40108f:	88 0c 24             	mov    %cl,(%rsp)
  401092:	48 8b 14 24          	mov    (%rsp),%rdx
  401096:	83 e2 0f             	and    $0xf,%edx
  401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx
  4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1)
  4010a4:	48 83 c0 01          	add    $0x1,%rax
  4010a8:	48 83 f8 06          	cmp    $0x6,%rax
  4010ac:	75 dd                	jne    40108b <phase_5+0x29>
```
在跳转到这段之前，%eax已经被设置为0。显而易见，这里是一个循环，循环条件为%rax != 0x6。所以，这里就是循环六次，每次都向栈中写入了一个字符，最后得到的就是"flyers"。但是，这些字符都是怎样产生的呢？

这里可以看到，都是从`0x4024b0(%rdx),%edx`得来的字符，用GDB调试发现:
```
(gdb) print (char*)0x4024b0
$2 = 0x4024b0 <array> "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"
```
这是一条更长的字符串，并且包括了"flyers"的所有字符，所以推测是从这个长字符串取出的字符。而这验证了我们的猜想:
```
 40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx
  40108f:	88 0c 24             	mov    %cl,(%rsp)
  401092:	48 8b 14 24          	mov    (%rsp),%rdx
  401096:	83 e2 0f             	and    $0xf,%edx
```
%rbx存放的是我们输入的字符串的地址，然后%rax是索引，每次从我们输入的字符串取出一个字符后，都会对其进行截断，只取最低4位存放在%edx中，然后这个%edx被用做了从长字符串取字符的索引。

现在我们明白了，就是我们输入的每个字符，都会取最低4位，然后转换成数字来作为长字符串 "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"的索引，最后依次取出的字符就是"flyers"！

所以，找出"flyers"在长字符串中的索引分别是:9, 15, 14, 5, 6, 7。为了凑成字符，对每个加上64恰好都是大写字母的ascii码:73, 79, 78, 69, 70, 71。

答案就是： IONEFG

## bomb 6
```
4010f4:	41 56                	push   %r14
4010f6:	41 55                	push   %r13
4010f8:	41 54                	push   %r12
4010fa:	55                   	push   %rbp
4010fb:	53                   	push   %rbx
4010fc:	48 83 ec 50          	sub    $0x50,%rsp
401100:	49 89 e5             	mov    %rsp,%r13
401103:	48 89 e6             	mov    %rsp,%rsi
401106:	e8 51 03 00 00       	callq  40145c <read_six_numbers>
40110b:	49 89 e6             	mov    %rsp,%r14
40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d
401114:	4c 89 ed             	mov    %r13,%rbp
401117:	41 8b 45 00          	mov    0x0(%r13),%eax
40111b:	83 e8 01             	sub    $0x1,%eax
40111e:	83 f8 05             	cmp    $0x5,%eax
401121:	76 05                	jbe    401128 <phase_6+0x34>
401123:	e8 12 03 00 00       	callq  40143a <explode_bomb>
```

这里又是用到了前面的`read_six_number`，所以我们的任务就是寻找6个数。开头这一部分代码的意思就是，必须每个数都要小于等于6。

```
401132:	44 89 e3             	mov    %r12d,%ebx
401135:	48 63 c3             	movslq %ebx,%rax
401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax
40113b:	39 45 00             	cmp    %eax,0x0(%rbp)
40113e:	75 05                	jne    401145 <phase_6+0x51>
401140:	e8 f5 02 00 00       	callq  40143a <explode_bomb>
401145:	83 c3 01             	add    $0x1,%ebx
401148:	83 fb 05             	cmp    $0x5,%ebx
40114b:	7e e8                	jle    401135 <phase_6+0x41>
40114d:	49 83 c5 04          	add    $0x4,%r13
401151:	eb c1                	jmp    401114 <phase_6+0x20>
401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi
401158:	4c 89 f0             	mov    %r14,%rax
40115b:	b9 07 00 00 00       	mov    $0x7,%ecx
401160:	89 ca                	mov    %ecx,%edx
401162:	2b 10                	sub    (%rax),%edx
401164:	89 10                	mov    %edx,(%rax)
401166:	48 83 c0 04          	add    $0x4,%rax
40116a:	48 39 f0             	cmp    %rsi,%rax
40116d:	75 f1                	jne    401160 <phase_6+0x6c>
```

首先，第一个元素被放在0x(%rbp)中，然后通过增加%ebx的值，循环了五次，也就是把数组后面的五个元素都和第一个元素进行比较，如果相同，就会触发炸弹。然后，同样有是一个循环，用来判断后面的元素依次不同。所以，这个部分就是用来判定每个数都是不同的。另外，对于栈中那六个数，每个数x，都被改成了7-x。

后面这部分就很复杂，刚开始一直没有看懂。然后发现有一个地址`0x6032d0`有被用到，所以用GDB打印它的内容可以发现:

```
(gdb) x 0x603320
0x603320 <node6>:       0x000001bb
```

再结合`mov    0x8(%rdx),%rdx`可以想到，这个应该是一个链表的结构，所以我们再打印出这个地址附近的信息:
```
(gdb) x /12xg 0x6032d0             
0x6032d0 <node1>:       0x000000010000014c      0x00000000006032e0
0x6032e0 <node2>:       0x00000002000000a8      0x00000000006032f0
0x6032f0 <node3>:       0x000000030000039c      0x0000000000603300
0x603300 <node4>:       0x00000004000002b3      0x0000000000603310
0x603310 <node5>:       0x00000005000001dd      0x0000000000603320
0x603320 <node6>:       0x00000006000001bb      0x0000000000000000
```

然后再分析代码，会发现把我们输入的数处理后，将其作为链表的索引，读取数据存放在栈中，然后又串连成了链表。

```
4011da:	bd 05 00 00 00       	mov    $0x5,%ebp
4011df:	48 8b 43 08          	mov    0x8(%rbx),%rax
4011e3:	8b 00                	mov    (%rax),%eax
4011e5:	39 03                	cmp    %eax,(%rbx)
4011e7:	7d 05                	jge    4011ee <phase_6+0xfa>
4011e9:	e8 4c 02 00 00       	callq  40143a <explode_bomb>
4011ee:	48 8b 5b 08          	mov    0x8(%rbx),%rbx
4011f2:	83 ed 01             	sub    $0x1,%ebp
4011f5:	75 e8                	jne    4011df <phase_6+0xeb>
```
这里遍历链表，要求链表的顺序是升序。所以，我们可以根据GDB打印出来的每一个结点的值，来找到数据的排列顺序为:3 4 5 6 1 2。然后，7-x后: 4 3 2 1 6 5。


后面还有隐藏关卡，但是天已经亮了，无力再战。。
```
0000000000401242 <secret_phase>:
  401242:	53                   	push   %rbx
  401243:	e8 56 02 00 00       	callq  40149e <read_line>
  401248:	ba 0a 00 00 00       	mov    $0xa,%edx
  40124d:	be 00 00 00 00       	mov    $0x0,%esi
  401252:	48 89 c7             	mov    %rax,%rdi
  401255:	e8 76 f9 ff ff       	callq  400bd0 <strtol@plt>
  40125a:	48 89 c3             	mov    %rax,%rbx
  40125d:	8d 40 ff             	lea    -0x1(%rax),%eax
  401260:	3d e8 03
   00 00       	cmp    $0x3e8,%eax
  401265:	76 05                	jbe    40126c <secret_phase+0x2a>
  401267:	e8 ce 01 00 00       	callq  40143a <explode_bomb>
  40126c:	89 de                	mov    %ebx,%esi
  40126e:	bf f0 30 60 00       	mov    $0x6030f0,%edi
  401273:	e8 8c ff ff ff       	callq  401204 <fun7>
  401278:	83 f8 02             	cmp    $0x2,%eax
  40127b:	74 05                	je     401282 <secret_phase+0x40>
  40127d:	e8 b8 01 00 00       	callq  40143a <explode_bomb>
  401282:	bf 38 24 40 00       	mov    $0x402438,%edi
  401287:	e8 84 f8 ff ff       	callq  400b10 <puts@plt>
  40128c:	e8 33 03 00 00       	callq  4015c4 <phase_defused>
  401291:	5b                   	pop    %rbx
  401292:	c3                   	retq   
  ```