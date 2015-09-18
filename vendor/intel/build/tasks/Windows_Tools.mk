# Define publish target for windows tools

winpub_tools := adb.exe AdbWinUsbApi.dll AdbWinApi.dll fastboot.exe
PUBLISH_WINDOWS_TOOLS_deps := $(addprefix $(HOST_OUT_EXECUTABLES)/,$(winpub_tools))

publish_windows_tools: $(PUBLISH_WINDOWS_TOOLS_deps) | $(ACP)
	@echo "Publish windows tools"
	$(hide) mkdir -p $(PUBLISH_TOOLS_PATH)/windows-x86/bin
	$(hide) $(ACP) $^ $(PUBLISH_TOOLS_PATH)/windows-x86/bin/



