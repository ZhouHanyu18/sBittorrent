********** zhy add *****************************
INCLUDEPATH  += $$PWD/../boost_1_60_0
LIBS += -L$$PWD/../boost_1_60_0/lib64-msvc-12.0

INCLUDEPATH  += $$PWD/../OpenSSL/include
LIBS += -L$$PWD/../OpenSSL/libs/ -lssleay32 -llibeay32


INCLUDEPATH  += $$PWD/../libtorrent/include
LIBS += -L$$PWD/../libtorrent/x64/Debug/ -llibtorrent
