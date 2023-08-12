# My_website

根据https://github.com/qinguoyi/TinyWebServer 来修改的服务器
主要解决了 为什么原来的server为什么 模拟proactor的性能明显好于reactor。 
原因：由于原来代码的设计问题，在reactor中线程池中读取数据的时候，如果出现错误无法处理，只能由主线程处理，所以主线程需要一个同步标识，只有当子线程读取完毕数据后，主线程才能能继续的循环，所以性能慢在这里。
解决方法：在每一个http_con中都设置的读取错误的处理方法，这样线程池中的线程出现错误后也可以自己处理，主线程不需要去等待子线程了。
修改后proactor与reactor的QPS相近。
