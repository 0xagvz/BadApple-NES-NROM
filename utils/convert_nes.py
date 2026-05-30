import argparse
import subprocess
import sys

CMD_WAIT      = 0x00
CMD_PPU_WRITE = 0x01
CMD_PPU_OFF   = 0x02
CMD_PPU_ON    = 0x03
CMD_END       = 0xFF

MAX_SIZE = (32 * 1024) - 1200 - 6 - 512  # ~30.5KB


def ffmpeg(inp, width, height, fps):
    vf = f"fps={fps},crop=in_h*{width}/{height}:in_h,scale={width}:{height}:flags=lanczos,format=gray"
    return [
        "ffmpeg", "-loglevel", "error",
        "-i", inp,
        "-vf", vf,
        "-f", "rawvideo",
        "-pix_fmt", "gray",
        "pipe:1",
    ]


def packbits_encode(data):
    data = list(data)
    out = []
    i = 0

    while i < len(data):
        run = 1
        while i + run < len(data) and data[i + run] == data[i] and run < 128:
            run += 1

        if run >= 3:
            out.extend(((257 - run) & 0xFF, data[i]))
            i += run
            continue

        literal = [data[i]]
        i += 1

        while i < len(data):
            run = 1
            while i + run < len(data) and data[i + run] == data[i] and run < 128:
                run += 1
            if run >= 3 or len(literal) >= 128:
                break
            literal.append(data[i])
            i += 1

        out.append((len(literal) - 1) & 0xFF)
        out.extend(literal)

    return out


def binarize(raw, threshold):
    return [1 if b > threshold else 0 for b in raw]


def frame_to_ppu_writes(frame, prev, base_addr, width, height):
    cmds = []

    for row in range(height):
        start = row * width
        end = start + width

        if prev is not None and frame[start:end] == prev[start:end]:
            continue

        addr = base_addr + row * 32
        cmds.extend([CMD_PPU_WRITE, (addr >> 8) & 0xFF, addr & 0xFF, width])
        cmds.extend(frame[start:end])

    return cmds


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("--threshold", type=int, default=128)
    parser.add_argument("--fps", type=int, default=2)
    parser.add_argument("--width", type=int, default=10)
    parser.add_argument("--height", type=int, default=8)
    args = parser.parse_args()

    width = args.width
    height = args.height
    fps = args.fps
    
    frame_size = width * height
    base_addr = 0x2000

    proc = subprocess.Popen(
        ffmpeg(args.input, width, height, fps),
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )

    raw_stream = [
        CMD_PPU_OFF,
        CMD_PPU_WRITE, 0x3F, 0x00, 0x04,
        0x0F, 0x30, 0x0F, 0x30,
    ]

    frames = 0
    prev_bin = None
    
    tiempo_acumulado = 0.0
    frames_por_cuadro_real = 60.0 / fps

    try:
        while True:
            raw = proc.stdout.read(frame_size)
            if len(raw) < frame_size:
                break

            cur_bin = binarize(raw, args.threshold)
            writes = frame_to_ppu_writes(cur_bin, prev_bin, base_addr, width, height)

            tiempo_acumulado += frames_por_cuadro_real
            frames_a_esperar_este_ciclo = int(tiempo_acumulado)
            tiempo_acumulado -= frames_a_esperar_este_ciclo
            
            wait_frames_exacto = frames_a_esperar_este_ciclo - 2
            if wait_frames_exacto < 0:
                wait_frames_exacto = 0

            candidate = raw_stream + [CMD_PPU_OFF] + writes + [CMD_PPU_ON, CMD_WAIT, wait_frames_exacto]
            test_enc = packbits_encode(candidate + [CMD_END])

            if len(test_enc) > MAX_SIZE:
                print(f"\nLimit reached at frame {frames}, size {len(test_enc)} bytes")
                break

            raw_stream = candidate
            prev_bin = cur_bin
            frames += 1

            if frames % 20 == 0:
                sys.stdout.write(
                    f"\r  frame {frames:4d} | {len(test_enc):5d}/{MAX_SIZE} bytes "
                    f"({len(test_enc) * 100 // MAX_SIZE}%)"
                )
                sys.stdout.flush()
    finally:
        proc.terminate()

    raw_stream.append(CMD_END)
    encoded = packbits_encode(raw_stream)

    with open("badapplevid.s", "w") as f:
        f.write('.segment "RODATA"\n')
        f.write('.export _badapplevid_bin\n')
        f.write('_badapplevid_bin:\n')
        for i in range(0, len(encoded), 16):
            chunk = encoded[i:i + 16]
            f.write("    .byte " + ", ".join(f"${b:02x}" for b in chunk) + "\n")



if __name__ == "__main__":
    main()