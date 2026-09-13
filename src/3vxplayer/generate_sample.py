#!/usr/bin/env python3
"""Generate the 300-frame FFmpeg test clip and encode it with sibling 3vt."""
import argparse
from pathlib import Path
import subprocess


def main():
    devkit = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--encoder', type=Path,
                        default=devkit.parent / '3vt/build/3vt_x86_64-linux')
    parser.add_argument('--ffmpeg', default='ffmpeg')
    args = parser.parse_args()
    media = devkit / 'build/3vxplayer-media'
    media.mkdir(parents=True, exist_ok=True)
    source = media / 'sample.avi'
    output = devkit / 'src/3vxplayer/takeme/3vxplayer_data/video.3vx'
    output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([
        args.ffmpeg, '-hide_banner', '-nostdin', '-y',
        '-f', 'lavfi', '-i', 'testsrc2=size=320x240:rate=30000/1001',
        '-f', 'lavfi', '-i',
        'aevalsrc=0.15*sin(2*PI*440*t)|0.15*sin(2*PI*660*t):s=22050:d=10.01',
        '-t', '10.01', '-c:v', 'rawvideo', '-pix_fmt', 'bgr24',
        '-c:a', 'pcm_s16le', '-ar', '22050', '-ac', '2', str(source)
    ], check=True)
    # Publish only a complete encode; retain the previous sample on failure.
    temporary = output.with_suffix('.3vx.tmp')
    try:
        subprocess.run([
            str(args.encoder.resolve()), 'to-3vx', str(source), str(temporary),
            '--preset', 'quality', '--video-quality', '100',
            '--codebook-plan-frames', '3', '--keyframe-interval', '45'
        ], check=True)
        subprocess.run([
            'python3', str(devkit.parent / '3vt/tools/verify_3vx_stream.py'),
            str(temporary)
        ], check=True)
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    print(output)


if __name__ == '__main__':
    main()
