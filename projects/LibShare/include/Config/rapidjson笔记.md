* 读取源
filereadstream(文件字节流)，MemoryStream(内存字节流), StringStream(字符串字符流)，istreamwrapper，
* 写入目标
filewritestream(文件字节流)，MemoryBuffer(内存字节流), StringBuffer(字符串字符流)，ostreamwrapper，

* encodedstream.h:把“字节”流(只能是char类型)转换为按某种编码的”字符“流，
字节流有filereadstream、filewritestream、MemoryStream、MemoryBuffer，另外的stream、stringbuffer、istreamwrapper、ostreamwrapper是字符流，是把已经按某种编码的字符串或stl的流,不改变编码,转成rapidjson方式的流。
使用AutoUTFInputStream需要用32位类型作为CharType，需要配合AutoUTF使用

* encodeings.h:编码参数类,用在上面转码或者需要指定编码的流中的模板参数.用于编码、解码，仿照Transcoder中的做法，可以实现字符编码的转换，这类的操作都是针对“字符”流，即：编码、解码的字符类型与输出、输入流的字符类型要一一对应(流式转码，windows的api是整体转码，处理大文件流式转码更有优势！)

* 常用类
    document文件：json数据类型
    reader：读取方法
    writer，prettywriter：写入方法

* 抽象概念：concept Handler：writer，prettywriter，document都是。
    reader接收一个Handler(Parse)，用document来构建json数据结构；
    GenericValue接收一个Handler(Accept)，用writer，rettywriter来写入json文本，用document来深拷贝json数据结构；

* StringRef，字符串的引用，不拷贝字符串
用字符串构造value时，有的需要传一个内存分配器，不给内存分配器的重载版本不会拷贝，如果源失效了，最后的结果也就失效了
