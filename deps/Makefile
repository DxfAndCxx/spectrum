all:
	tar xvf LuaJIT-2.1.0-beta2.tar.gz
	make -C LuaJIT-2.1.0-beta2
	make -C LuaJIT-2.1.0-beta2 install PREFIX=$(PREFIX)
	tar xvf jansson-2.7.tar.gz
	cd jansson-2.7; ./configure --prefix=$(PREFIX)
	make -C jansson-2.7
	make -C jansson-2.7 install
	make -C monip
	make -C monip install

