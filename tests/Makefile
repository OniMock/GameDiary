# -------------------------------------------------------------
#  GameDiary Automated Test Suite Makefile
# -------------------------------------------------------------

CC ?= gcc
CFLAGS ?= -O0 -g -Wall -Wextra -Wno-unused-parameter -std=c99
INCLUDES = -Itests/mock_pspsdk -Iinclude -Iinclude/app -Ilib -Itests
DEFINES = -DGDIARY_PLUGIN -D_PSP_FW_VERSION=661 -DTESTING=1
LIBS = -lm

ifeq ($(COVERAGE), 1)
CFLAGS += -fprofile-arcs -ftest-coverage
LIBS += -lgcov
endif

BUILD_DIR ?= tests/build
BIN_DIR ?= tests/bin
TARGET ?= $(BIN_DIR)/run_tests

SRCS = \
	tests/main.c \
	tests/framework/mock_pspsdk.c \
	tests/framework/mock_stubs.c \
	tests/unit/test_db_schema.c \
	tests/unit/test_utils.c \
	tests/unit/test_sfo_parser.c \
	tests/unit/test_apitype.c \
	tests/unit/test_plugin_config.c \
	tests/unit/test_tracker_db.c \
	tests/unit/test_tracker_runtime.c \
	tests/unit/test_input_combo.c \
	tests/unit/test_storage.c \
	tests/unit/test_app_config.c \
	tests/unit/test_app_plugin_dat.c \
	tests/unit/test_game_category.c \
	tests/unit/test_version_check.c \
	tests/unit/test_carousel_state.c \
	tests/unit/test_data_loader.c \
	tests/unit/test_data_backup.c \
	tests/unit/test_stats_calculator.c \
	tests/unit/test_i18n.c \
	tests/unit/test_metadata_repository.c \
	tests/unit/test_overlay_blit.c \
	tests/unit/test_tracker_time_accounting.c \
	tests/integration/test_integration_tracker_pipeline.c \
	tests/integration/test_integration_storage_recovery.c \
	tests/integration/test_integration_data_lifecycle.c \
	src/common/utils.c \
	src/common/sfo_parser.c \
	src/common/storage.c \
	src/common/psp_hardware.c \
	src/common/debug.c \
	src/plugin/apitype.c \
	src/plugin/plugin_config.c \
	src/plugin/tracker_db.c \
	src/plugin/tracker_runtime.c \
	src/plugin/input_combo.c \
	src/plugin/icon_capture.c \
	src/plugin/metadata_repository.c \
	src/plugin/detector.c \
	src/plugin/overlay_blit.c \
	src/plugin/overlay_msx_font.c \
	src/app/config/config.c \
	src/app/config/plugin_dat.c \
	src/app/data/game_category.c \
	src/app/data/stats_calculator.c \
	src/app/data/data_loader.c \
	src/app/data/data_backup.c \
	src/app/network/version_check.c \
	src/app/ui/carousel_state.c \
	src/app/i18n/i18n.c \
	src/app/i18n/i18n_en.c \
	src/app/i18n/i18n_pt.c \
	src/app/i18n/i18n_es.c \
	src/app/i18n/i18n_ru.c \
	src/app/i18n/i18n_jp.c \
	src/app/i18n/i18n_cn.c \
	src/app/i18n/i18n_de.c \
	src/app/i18n/i18n_it.c \
	src/app/i18n/i18n_fr.c \
	lib/cJSON/cJSON.c

OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all clean test coverage

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

test: $(TARGET)
	@./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
