TEMPLATE	= 	app
CONFIG		+= 	qt warn_off release thread

INCLUDEPATH += . include /usr/include/qt 

HEADERS 	+=	*.h
HEADERS 	+=	include/*.h

SOURCES 	+=	*.cpp

win32:LIBS	=	-nodefaultlib:"msvcrt" xbase.lib gds32_ms.lib
#unix:LIBS	=	-lxbase -lgds -lcrypt -ldl -lsqlite3
unix:LIBS	=	-lxbase -lgds -lcrypt -ldl -lcrypto

TARGET		= 	Kacca
DEFINES		=	IBASE QT_THREAD_SUPPORT
#REQUIRES	=	full-config

# this is not critical, but needed for qtcreator
INCLUDEPATH += /usr/include F:/Qt/qt-3.3.x-p8/include F:/Qt/xbase /home/slair/include/qt-3.3.x-p8/include /home/slair/include
