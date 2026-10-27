"""Decode every audio asset and check format, headroom and loop boundaries."""
import argparse
import json
from pathlib import Path
import subprocess
import numpy as np


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory",type=Path)
    parser.add_argument("--manifest",type=Path,required=True)
    parser.add_argument("--report",type=Path,required=True)
    args=parser.parse_args()
    manifest=json.loads(args.manifest.read_text(encoding="utf-8"))
    results=[]
    for asset in manifest["assets"]:
        path=args.directory/asset["file"]
        meta=json.loads(subprocess.check_output(["ffprobe","-v","error","-show_streams","-of","json",str(path)]))["streams"][0]
        assert int(meta["sample_rate"])==44100 and meta["channels"]==2,path
        raw=subprocess.check_output(["ffmpeg","-v","error","-i",str(path),"-f","f32le","-"])
        data=np.frombuffer(raw,dtype="<f4").reshape(-1,2)
        peak=float(np.max(np.abs(data)));rms=float(np.sqrt(np.mean(data**2)))
        assert np.isfinite(data).all() and .002<rms<.3 and peak<.9,(path,rms,peak)
        assert abs(len(data)/44100-asset["seconds"])<.06,path
        boundary=float(np.max(np.abs(data[0]-data[-1])))
        if asset["loop"]:
            ordinary=float(np.sqrt(np.mean(np.diff(data,axis=0)**2)))
            assert boundary<max(.025,ordinary*8),(path,boundary,ordinary)
        else:
            assert max(float(np.max(np.abs(data[0]))),float(np.max(np.abs(data[-1]))))<.003,path
        results.append({"file":path.name,"seconds":round(len(data)/44100,4),"peak_dbfs":round(20*np.log10(peak),2),"rms_dbfs":round(20*np.log10(rms),2),"boundary_delta":round(boundary,6)})
    args.report.write_text(json.dumps({"assets":results,"passed":True},indent=2)+"\n",encoding="utf-8")
    print(f"Decoded and checked {len(results)} assets: 44.1 kHz stereo, no clipping, valid durations and boundaries.")

if __name__=="__main__":main()
