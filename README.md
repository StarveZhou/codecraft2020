# codecraft2020

***尽量不要删除和修改已有的代码，创建新的代码。***

## 使用Makefile

`make F=main.cc main cmp` 编译之后检验结果

### 编译并运行代码

`make F=xxx`

`xxx`为代码的文件名，代码文件为`xxx`

更改宏`INPUT_PATH`来更改输入的文件
结果输出到`test_output.txt`文件中

### 检验结果

`make cmp`

直接对比`test_output.txt`和`resources/result.txt`

使用`utls/cmp.py`，默认上述参数，或者第一个参数为输出结果，第二个参数为标准结果

### 清除多余文件

`make clean`