override TARGET = keychron_q3max_custom
VIA_ENABLE = yes
SPAM_MACRO_ENABLE = yes
OPT_DEFS += -DSPAM_MACRO_ENABLE
SRC += pilot_remap.c spam_config.c modes.c
