循环展开
拓扑排序
点的可行性判断 *
能用bool的地方使用bool
考虑还有什么可以存为结构体，缓存加速
改成mmap *
看代码，有啥能够缩减的地方
Answer分类 *
排序用long long


输出答案的时候用mmap，先对所有答案排序，再分成4份
序列化的长度用char表示

Answer不分类，大小固定
变量开在最前面
多进程io代替多线程
https://github.com/justarandomstring/2020-Huawei-Code-Craft/blob/master/Warm-up/main.cpp

search里面的所有参数用到的时候再算，比如size_abc
c->d的边挂在b->c的边上

多线程归并，就是先不memcpy，在（不是归并排序

整理内存，多用cache！！！

不用的边和点shrink