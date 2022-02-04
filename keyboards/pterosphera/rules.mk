EXTRAFLAGS += -flto

# MCU name
MCU = atmega32u4

# Build Options
#   change yes to no to disable
#
BOOTMAGIC_ENABLE = no		# Enable Bootmagic Lite
MOUSEKEY_ENABLE = yes		# Mouse keys
EXTRAKEY_ENABLE = no		# Audio control and System control
CONSOLE_ENABLE = no			# Console for debug
COMMAND_ENABLE = no 		# Commands for debug and configuration
NKRO_ENABLE = yes           # Enable N-Key Rollover
BACKLIGHT_ENABLE = no		# Enable keyboard backlight functionality
AUDIO_ENABLE = no           # Audio output

SPLIT_KEYBOARD = yes
POINTING_DEVICE_ENABLE = yes

DEFAULT_FOLDER = pterosphera/v1

RGBLIGHT_ENABLE = yes       # Enable keyboard RGB underglow
