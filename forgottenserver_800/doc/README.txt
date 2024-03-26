[ README
	Project Name
		The Forgotten Server

	Version
		0.3

	Codenamed
		Crying Damson

	License
		GNU GPLv3

	Forum
		http://otland.net/
]

[ CREDITS
	First of all, credits to the developers of this project, found in the
	DEVELOPERS part of this file.

	This server is based on the OpenTibia project which is developed by:
		http://sourceforge.net/project/memberlist.php?group_id=32523

	.. also a thanks to the following people who has helped this
	project by contributing something: Cayan, TheMask, slawkens
		Rafael Hamdan, Fare-Fray, Kiwi Dan, Gentleman Riko,
		Piotrek1447, Hermes, Gesior.pl, levandi, Bennet,
		Casa, Winghawk, Nexoz, Godely, buzzbuzz2, koob, tryller.
]

[ LICENSE
	This project is licensed under GPLv3, you should have received a copy
	of this license with this project.
]

[ DEVELOPERS
	Project Manager(s)
		Talaturen
		Elf

	Founder(s)
		Talaturen
		Kiper

	C++ Programmer(s)
		Elf
		Talaturen

	LUA Scripter(s)
		Elf

	DATA Directory Manager(s)
		Lithium
		KaczooH

	Documentation Manager(s)
		KaczooH
]

[ GITHUB
	Mainly english:
		https://github.com/tryller/otserv/
]

[ SUPPORT
	Having trouble with any of the instructions, or is there
	anything you just want to ask?
	Create a new thread and ask here:
		https://github.com/tryller/otserv/issues
]

[ BUGS / PATCHES / FEATURE REQUESTS
	https://github.com/tryller/otserv/issues
]

[ COMPILING
	GNU\Linux - Debian (...and forks, eg. Ubuntu)
		A detailed and always up-to-date tutorial may be found here:
			http://otland.net/f137/linux-ultimate-compile-guide-debian-ubuntu-included-2868/

		Root
			You need to get on root user or use sudo for almost all below operations.

		Download required packages
			Launch a terminal, and paste in the following commands:
				apt-get install libboost1.38-dev libboost-system1.38-dev libboost-filesystem1.38-dev libboost-date-time1.38-dev libboost-regex1.38-dev libboost-thread1.38-dev libgmp3-dev liblua5.1-0 liblua5.1-0-dev liblua50 liblua50-dev liblualib50 liblualib50-dev lua50 lua5.1 libsqlite0-dev libsqlite3-dev sqlite3 libmysql++-dev libmysqlclient15-dev mysql-client-5.0 mysql-server-5.0 mysql-common libxml2-dev libxml++2.6-dev cpp gcc g++ make automake autoconf pkg-config subversion liblua5.1-sql-mysql-dev liblua5.1-sql-sqlite3-dev zlib1g-dev zlib1g

		Download sources
			Goto https://github.com/tryller/otserv and click "Clone or Download":
				click download ZIP.

		Compiling
			Launch a terminal, and type this:
				cd forgottenserver/800 && ./autogen.sh && ./configure --enable-sqlite --enable-server-diag && make

		Linker error
			If you receive a linker error saying something with 'libboost', try pasting the following commands to terminal and then move back to compiling again:
				wget http://garr.dl.sourceforge.net/sourceforge/asio/boost_asio_1_0_0.tar.gz
				tar -xzf boost_asio_1_0_0.tar.gz
				cp -ar boost_asio_1_0_0/boost/* /usr/include/boost/
				g++ boost_asio_1_0_0/libs/system/src/error_code.cpp -c -o /usr/lib/libboost_system.a
				ln /usr/lib/libboost_regex-mt.so /usr/lib/libboost_regex.so
				ln /usr/lib/libboost_regex-mt.a /usr/lib/libboost_regex.a
				ln /usr/lib/libboost_thread-mt.so /usr/lib/libboost_thread.so
				ln /usr/lib/libboost_thread-mt.a /usr/lib/libboost_thread.a
				ln /usr/lib/libboost_filesystem-mt.so /usr/lib/libboost_filesystem.so
				ln /usr/lib/libboost_filesystem-mt.a /usr/lib/libboost_filesystem.a
				ln /usr/lib/libboost_date_time-mt.so /usr/lib/libboost_date_time.so
				ln /usr/lib/libboost_date_time-mt.a /usr/lib/libboost_date_time.a
				rm -rf boost_asio_1_0_0 boost_asio_1_0_0.tar.gz

	Windows
		Dev-Cpp
			Download TheForgottenDev-Cpp and the sources from:
				http://otland.net/showthread.php?t=1024

			Extract them anywhere and then run devcpp.exe, press on the menuitem
			"File -> Open Project or File" and choose C:\path\to\sources\dev-cpp\TheForgottenServer.dev.

			At this point Dev-Cpp should be running with the project, now you can press
			"Execute -> Rebuild All", if there is any error, then report it here:
				http://otland.net/showthread.php?t=379

			After it get compiled, copy theforgottenserver.exe from C:\path\to\sources\dev-cpp to directory above (C:\path\to\sources)
]

[ CONFIGURING
	Launch config.lua with your favorite text editor (NOTE: Windows built-in Notepad
	may not display line break!) and start configuring it (if you would need help or
	you wouldn't understand something, please check doc/CONFIG_HELP), in case there
	are other things you would like to configure- go over to the data directory.
]

[ EXECUTING
	Linux
		MySQL
			Read MYSQL_HELP then launch a terminal, and paste this command:
				cd ~/forgottenserver/forgottenserver/ && ./theforgottenserver

		SqLite
			Launch a terminal, and paste this command:
				cd ~/forgottenserver/forgottenserver/ && ./theforgottenserver

	Windows
		MySQL
			Read MYSQL_HELP and then move the exe file to the directory where you have
			config.lua and data directory and execute it.

		SqLite
			Move the exe file to the directory where you have config.lua and data directory
			and execute it.
]
