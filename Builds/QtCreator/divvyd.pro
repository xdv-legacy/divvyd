
# Divvy protocol buffers

PROTOS = ../../src/divvy_data/protocol/divvy.proto
PROTOS_DIR = ../../build/proto

# Google Protocol Buffers support

protobuf_h.name = protobuf header
protobuf_h.input = PROTOS
protobuf_h.output = $${PROTOS_DIR}/${QMAKE_FILE_BASE}.pb.h
protobuf_h.depends = ${QMAKE_FILE_NAME}
protobuf_h.commands = protoc --cpp_out=$${PROTOS_DIR} --proto_path=${QMAKE_FILE_PATH} ${QMAKE_FILE_NAME}
protobuf_h.variable_out = HEADERS
QMAKE_EXTRA_COMPILERS += protobuf_h

protobuf_cc.name = protobuf implementation
protobuf_cc.input = PROTOS
protobuf_cc.output = $${PROTOS_DIR}/${QMAKE_FILE_BASE}.pb.cc
protobuf_cc.depends = $${PROTOS_DIR}/${QMAKE_FILE_BASE}.pb.h
protobuf_cc.commands = $$escape_expand(\\n)
#protobuf_cc.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += protobuf_cc

# Divvy compilation

DESTDIR = ../../build/QtCreator
OBJECTS_DIR = ../../build/QtCreator/obj

TEMPLATE = app
CONFIG += console thread warn_off
CONFIG -= qt gui

DEFINES += _DEBUG

linux-g++:QMAKE_CXXFLAGS += \
    -Wall \
    -Wno-sign-compare \
    -Wno-char-subscripts \
    -Wno-invalid-offsetof \
    -Wno-unused-parameter \
    -Wformat \
    -O0 \
    -std=c++11 \
    -pthread

INCLUDEPATH += \
    "../../src/leveldb/" \
    "../../src/leveldb/port" \
    "../../src/leveldb/include" \
    $${PROTOS_DIR}

OTHER_FILES += \
#   $$files(../../src/*, true) \
#   $$files(../../src/beast/*) \
#   $$files(../../src/beast/modules/beast_basics/diagnostic/*)
#   $$files(../../src/beast/modules/beast_core/, true)

UI_HEADERS_DIR += ../../src/divvy_basics

# ---------
# New style
#
SOURCES += \
    ../../src/divvy/beast/divvy_beast.unity.cpp \
    ../../src/divvy/beast/divvy_beastc.c \
    ../../src/divvy/common/divvy_common.unity.cpp \
    ../../src/divvy/http/divvy_http.unity.cpp \
    ../../src/divvy/json/divvy_json.unity.cpp \
    ../../src/divvy/peerfinder/divvy_peerfinder.unity.cpp \
    ../../src/divvy/radmap/divvy_radmap.unity.cpp \
    ../../src/divvy/resource/divvy_resource.unity.cpp \
    ../../src/divvy/sitefiles/divvy_sitefiles.unity.cpp \
    ../../src/divvy/sslutil/divvy_sslutil.unity.cpp \
    ../../src/divvy/testoverlay/divvy_testoverlay.unity.cpp \
    ../../src/divvy/types/divvy_types.unity.cpp \
    ../../src/divvy/validators/divvy_validators.unity.cpp

# ---------
# Old style
#
SOURCES += \
    ../../src/divvy_app/divvy_app.unity.cpp \
    ../../src/divvy_app/divvy_app_pt1.unity.cpp \
    ../../src/divvy_app/divvy_app_pt2.unity.cpp \
    ../../src/divvy_app/divvy_app_pt3.unity.cpp \
    ../../src/divvy_app/divvy_app_pt4.unity.cpp \
    ../../src/divvy_app/divvy_app_pt5.unity.cpp \
    ../../src/divvy_app/divvy_app_pt6.unity.cpp \
    ../../src/divvy_app/divvy_app_pt7.unity.cpp \
    ../../src/divvy_app/divvy_app_pt8.unity.cpp \
    ../../src/divvy_basics/divvy_basics.unity.cpp \
    ../../src/divvy_core/divvy_core.unity.cpp \
    ../../src/divvy_data/divvy_data.unity.cpp \
    ../../src/divvy_hyperleveldb/divvy_hyperleveldb.unity.cpp \
    ../../src/divvy_leveldb/divvy_leveldb.unity.cpp \
    ../../src/divvy_net/divvy_net.unity.cpp \
    ../../src/divvy_overlay/divvy_overlay.unity.cpp \
    ../../src/divvy_rpc/divvy_rpc.unity.cpp \
    ../../src/divvy_websocket/divvy_websocket.unity.cpp

LIBS += \
    -lboost_date_time-mt\
    -lboost_filesystem-mt \
    -lboost_program_options-mt \
    -lboost_regex-mt \
    -lboost_system-mt \
    -lboost_thread-mt \
    -lboost_random-mt \
    -lprotobuf \
    -lssl \
    -lrt
