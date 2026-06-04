.PHONY: all clean video

all: badapple.nes

badapple.nes: badapple.o badapplevid.o watermark.o
	ld65 -C /usr/share/cc65/cfg/nes.cfg -o badapple.nes.tmp badapple.o badapplevid.o watermark.o /usr/share/cc65/lib/nes.lib
	python3 -c "f=open('badapple.nes.tmp','r+b'); f.seek(5); f.write(b'\\x00'); f.seek(0); sz=16+f.read(16)[4]*16384; f.truncate(sz); f.close()"
	mv badapple.nes.tmp badapple.nes
	@echo "Generated badapple.nes"

badapple.s: badapple.c
	cc65 -t nes -O -o badapple.s badapple.c

badapple.o: badapple.s
	ca65 -o badapple.o badapple.s

badapplevid.o: badapplevid.s
	ca65 -o badapplevid.o badapplevid.s

badapplevid.s: badapple.bin
	printf '.segment "RODATA"\n.export _video_bin\n.export _video_bin_end\n_video_bin:\n.incbin "badapple.bin"\n_video_bin_end:\n' > badapplevid.s

watermark.o: watermark.s
	ca65 -o watermark.o watermark.s

badapple.bin: utils/BadAppleEncoder.py utils/badapple.mp4
	python3 ./utils/BadAppleEncoder.py utils/badapple.mp4

video: badapple.bin badapplevid.s

clean:
	rm -f badapple.s badapple.o badapplevid.s badapplevid.o watermark.o *.tmp *.nes *.bin
