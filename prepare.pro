TEMPLATE = aux

OUT_PATH = $$OUT_PWD
SRC_PATH = $${OUT_PATH}/apng/plugins
DST_PATH = $${OUT_PATH}/bin

win32 {
    SRC_PATH ~= s,/,\\,g # fix slashes
    DST_PATH ~= s,/,\\,g # fix slashes
}

copydata.commands = $(COPY_DIR) $${SRC_PATH} $${DST_PATH}
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata
