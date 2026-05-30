.PHONY: clean video

all: badapple.nes

badapple.nes: badapplevid.s badapple.o badapplevid.o
	ld65 -C /usr/share/cc65/cfg/nes.cfg -o badapple.nes.tmp badapple.o badapplevid.o /usr/share/cc65/lib/nes.lib
	python3 -c "f=open('badapple.nes.tmp','r+b'); f.seek(5); f.write(b'\x00'); f.seek(0); sz=16+f.read(16)[4]*16384; f.truncate(sz); f.close()"
	mv badapple.nes.tmp badapple.nes
	find . -maxdepth 1 -name "*.s" ! -name "badapplevid.s" -delete
	@echo "Generated badapple.nes"

badapple.s: badapple.c
	cc65 -t nes -O -o badapple.s badapple.c

badapple.o: badapple.s
	ca65 -o badapple.o badapple.s

badapplevid.o: badapplevid.s
	ca65 -o badapplevid.o badapplevid.s

badapplevid.s:
	@echo "Error: badapplevid.s not found. Run 'make video' first."
	@false

video:
	python3 utils/convert_nes.py utils/badapplevid.mp4 --width 10 --height 9 --fps 1

clean:
	find . -maxdepth 1 \( -name "*.s" -o -name "*.o" -o -name "*.tmp" -o -name "*.nes" \) ! -name "badapplevid.s" -delete