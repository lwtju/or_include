用处
----
1、过滤器用于大的集合查找是否某元素在集合中，为了增强准确度和衡量集合占据的空间大小,cuckoo filter相比较于bloom filter表现的更加出色。  
2、可以异步生成元素集合文件，根据元素个数生成不同文件大小的集合，具体可以调整resize_line函数。  
3、文件可以直接派发到各个webserver上供使用（可以考虑inotify+rsync），可以考虑直接加载内存，或者运用代码中的方法通过调用mmap做文件映射，酌情使用。

用法
----
1、生成一个制造集合文件的可执行文件cf   

    gcc -std=c99 -lm cuckoo_filter_main.c -o cf  

2、生成数据集合文件  

	./cf inputfile  outputfile

3、调用group.lua模块

	function _M.checkIn(self, groupName, key)  
注：函数中groupName要和文件名字对应, key为相应的元素
