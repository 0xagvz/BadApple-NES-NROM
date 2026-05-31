import subprocess
import cv2
import numpy as np
import os
import argparse

preprocessedPath = "badapple-preprocessed-temp.mp4"
outputBin = "badapple.bin"


def ffmpeg(input_path, output_path, fps):
    subprocess.run([
        "ffmpeg",
        "-i", input_path,

        "-vf", f"fps={fps},format=gray",

        "-an",
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-crf", "0",
        "-pix_fmt", "gray",

        "-y",
        output_path
    ])



def videoToBin(input_path, output_path, width=8, height=6, threshold=128):
    cap = cv2.VideoCapture(input_path)

    if not cap.isOpened():
        raise ValueError(f"No se pudo abrir el video: {input_path}")


    with open(output_path, "wb") as f:
        while True:
            ret, frame = cap.read()
            if not ret:
                break

            frame = cv2.resize(frame, (width, height), interpolation=cv2.INTER_NEAREST)

            if len(frame.shape) == 3:
                gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            else:
                gray = frame

            _, bw = cv2.threshold(gray, threshold, 1, cv2.THRESH_BINARY)

            pad = (8 - (width % 8)) % 8
            if pad:
                bw = np.pad(bw, ((0, 0), (0, pad)), mode="constant")

            packed = np.packbits(bw.astype(np.uint8), axis=1, bitorder="big")

            f.write(packed.tobytes())

    cap.release()
    os.remove(preprocessedPath)

def main():
    argparser = argparse.ArgumentParser(description="Encode a video into a binary format for NES display.")
    argparser.add_argument('--video_path', help='Path to the input video file.', default=None)
    argparser.add_argument('video_path_pos', nargs='?', help='Path to the input video file (positional).')
    argparser.add_argument("--fps", type=int, default=5, help="Frames per second for the output video (default: 5).")
    argparser.add_argument("--width", type=int, default=16, help="Width of the output frames in pixels (default: 16).")
    argparser.add_argument("--height", type=int, default=12, help="Height of the output frames in pixels (default: 12).")

    args = argparser.parse_args()

    video = args.video_path or args.video_path_pos
    if not video:
        argparser.error('missing video path (positional or --video_path)')

    ffmpeg(video, preprocessedPath, fps=args.fps)
    videoToBin(preprocessedPath, outputBin, width=args.width, height=args.height)


if __name__ == '__main__':
    main()
