
# 编译
1.修改Makefile 指定编译器
2.make 即可

# 使用方案1 

1.修改 socks5.conf 设置端口,listenport=5038,认证加密方案ea_method=0:无用户密码模式
在linux上执行 ./socks5 -f socks5.conf

使用火狐浏览器配置socks5代理即可  linux的IP和刚才设置的端口




# 使用方案2
windows 系统代理socks4


原理windows本地应用访问本机 socks4服务
然后server转发流量远端linux服务器上的socks5代理服务器


启动[windows-s4/socks.exe] socks4服务监听5039
修改windows系统代理   socks://127.0.0.1   5039


1.修改 socks5.conf 设置端口,listenport=5038,认证加密方案ea_method=129:自定义加密
在linux上执行 ./socks5 -f socks5.conf










