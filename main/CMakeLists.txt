file(GLOB srcs
  "hc/object/*.cpp"
  "hc/device/*.cpp"
  "hc/device/ble/*.cpp"
  "hc/device/ble/module/*.cpp"
  "hc/gateway/*.cpp"
  "hc/json/*.cpp"
  "hc/util/*.cpp"
  "hc/protocol/qr/*.cpp"
  "hc/protocol/relay/*.cpp"
  "hc/protocol/i2c/*.cpp"
  "hc/protocol/i2c/*.c"
  "hc/product/*.cpp"
  "log/*.cpp"
  "button/*.cpp"
  "mqtt/*.cpp"
  "mqtt/*.c"
  "led/*.cpp"
  "uart/*.cpp"
  "wifi/*.cpp"
  "sntp/*.cpp"
  "sntp/*.c"
  "config/*.cpp"
  "*.cpp"
)

set(incs
	"hc"
	"hc/json"
	"hc/object"
	"hc/gateway"
	"hc/device"
	"hc/device/ble"
	"hc/util"
	"hc/protocol/qr"
	"hc/protocol/relay"
	"hc/protocol/i2c"
	"hc/product"
	"button"
	"mqtt"
	"led"
	"log"
	"uart"
	"wifi"
	"sntp"
	"config")

if(CONFIG_BLE_MESH)
	list(APPEND srcs
		"protocol/ble/BleProtocol.cpp"
		"protocol/ble/esp_ble.c"
		"protocol/ble/ble_mesh_example_init.c")

	list(APPEND incs
		"protocol/ble")
else()
	list(APPEND srcs
		"hc/protocol/ble/BleProtocol.cpp")

	list(APPEND incs
		"hc/protocol/ble")
endif()

idf_component_register(SRCS "main.cpp" "${srcs}"
	INCLUDE_DIRS "${incs}")

# spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
target_compile_options(${COMPONENT_LIB} PRIVATE "-std=gnu++17")

component_compile_options(-Wno-unused-variable -Wno-unused-function)
component_compile_options(-Wno-missing-field-initializers)
component_compile_options(-Wno-deprecated-declarations)
component_compile_options(-Wno-error=format= -Wno-format)
