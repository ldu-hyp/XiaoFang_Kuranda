idf_component_register(
    SRCS
        "main.cpp"
        "app/Application.cpp"
        "ui/Ui.cpp"

        "display.c"
        "imu.c"
        "input.c"
        "buzzer.c"
        "storage.c"
        "power.c"
        "network.c"
        "modem.c"

        "games/GameManager.cpp"
        "games/HourglassGame.cpp"
        "games/DiceGame.cpp"
        "games/BaguaGame.cpp"
        "games/SnakeGame.cpp"
        "games/MazeGame.cpp"
        "games/CubeManGame.cpp"
        "games/SokobanGame.cpp"
        "games/DodgeGame.cpp"
        "games/PongGame.cpp"

    INCLUDE_DIRS
        "."
        "app"
        "ui"
        "games"

    REQUIRES
        driver
        esp_adc
        esp_event
        esp_netif
        esp_timer
        esp_wifi
        nvs_flash

    PRIV_REQUIRES
        led_strip
)

# Embedded C++ policy: use classes/RAII/type safety, but keep runtime behavior
# deterministic and avoid exceptions/RTTI. No dynamic allocation is used by
# the application/game layer.
target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)
