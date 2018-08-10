demo : src/main.c
	/opt/rk3308/host/bin/arm-rockchip-linux-gnueabihf-gcc -g -o demo -Werror -L /home/junlon2006/workspace/github/360-wrapper/lib src/uni_log.c src/uni_lasr.c src/main.c src/uni_dsp.c src/list_head.c src/uni_audio_player.c src/uni_databuf.c src/uni_tts_offline_process.c src/uni_tts_online_process.c src/uni_tts_config.c src/uni_tts_player.c src/uni_tcp_ser.c src/uni_config.c src/cJSON.c -fPIC -lpthread -lsyshal -lUniLinearMicArray -lengine_c -lavcodec -lavformat-54 -lavutil-52 -lswresample-0 -lm

