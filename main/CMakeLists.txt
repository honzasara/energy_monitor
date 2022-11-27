set(srcs "main.cpp" "menu.cpp" "SSD1306.cpp"   "saric_utils_esp32.cpp" "EEPROM.cpp" "esp32_saric_mqtt_network.cpp" "stdlib_noniso.c" "WString.cpp"
	 "ezButton.cpp" "saric_energy.cpp"
	)

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS ".")
